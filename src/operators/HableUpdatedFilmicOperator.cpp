/*
    Copyright (c) 2022 Tizian Zeltner

    tonemapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class HableUpdatedFilmicOperator : public TonemapOperator {
public:
    HableUpdatedFilmicOperator() : TonemapOperator() {
        name = "Hable (Updated) Filmic";
        description = R"(Filmic curve by John Hable. Based on the original
            version from the "Filmic Tonemapping for Real-time Rendering"
            SIGGRAPH 2010 course, but updated with a better controllability. See
            his blog post "Filmic Tonemapping with Piecewise Power Curves")";

        /* See also the open source implementation by John Hable:
           https://github.com/johnhable/fw-public */
        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float tStr;
            uniform float tLen;
            uniform float sStr;
            uniform float sLen;
            uniform float sAngle;

            vec2 asSlopeIntercept(float x0, float x1, float y0, float y1) {
                float m, b;
                float dy = (y1 - y0),
                      dx = (x1 - x0);
                if (dx == 0.0) {
                    m = 1.0;
                } else {
                    m = dy / dx;
                }
                b = y0 - x0*m;
                return vec2(m, b);
            }

            float evalDerivativeLinearGamma(float m, float b, float g, float x) {
                return g * m * pow(m * x + b, g - 1.0);
            }

            vec2 solveAB(float x0, float y0, float m) {
                float B = (m * x0) / y0,
                      lnA = log(y0) - B * log(x0);
                return vec2(lnA, B);
            }

            float evalCurveSegment(float x, float offsetX, float offsetY, float scaleX, float scaleY, float lnA, float B) {
                float x0 = (x - offsetX) * scaleX,
                      y0 = 0.0;
                if (x0 > 0.0) {
                    y0 = exp(lnA + B * log(x0));
                }
                return y0 * scaleY + offsetY;

            }

            void main() {
                // Convert from "user" to "direct" parameters
                float tLen_      = pow(tLen, 2.2),
                      x0         = 0.5 * tLen_,
                      y0         = (1.0 - tStr) * x0,
                      remainingY = 1.0 - y0,
                      initialW   = x0 + remainingY,
                      y1Offset   = (1.0 - sLen) * remainingY,
                      x1         = x0 + y1Offset,
                      y1         = y0 + y1Offset,
                      extraW     = pow(2.0, sStr) - 1.0,
                      W          = initialW + extraW,
                      overshootX = (2.0 * W) * sAngle * sStr,
                      overshootY = 0.5 * sAngle * sStr,
                      invGamma   = 1.0 / gamma;

                // Precompute information for all three segments (mid, toe, shoulder)
                float curveWinv = 1.0 / W;
                x0 /= W;
                x1 /= W;
                overshootX /= W;

                vec2 tmp = asSlopeIntercept(x0, x1, y0, y1);
                float m = tmp.x,
                      b = tmp.y,
                      g = invGamma;

                float midOffsetX = -(b / m),
                      midOffsetY = 0.0,
                      midScaleX  = 1.0,
                      midScaleY  = 1.0,
                      midLnA = g * log(m),
                      midB = g;

                float toeM      = evalDerivativeLinearGamma(m, b, g, x0),
                      shoulderM = evalDerivativeLinearGamma(m, b, g, x1);

                y0 = max(1e-5, pow(y0, invGamma));
                y1 = max(1e-5, pow(y1, invGamma));
                overshootY = pow(1.0 + overshootY, invGamma) - 1.0;

                tmp = solveAB(x0, y0, toeM);

                float toeOffsetX = 0.0,
                      toeOffsetY = 0.0,
                      toeScaleX  = 1.0,
                      toeScaleY  = 1.0,
                      toeLnA     = tmp.x,
                      toeB       = tmp.y;

                float shoulderX0 = (1.0 + overshootX) - x1,
                      shoulderY0 = (1.0 + overshootY) - y1;
                tmp = solveAB(shoulderX0, shoulderY0, shoulderM);

                float shoulderOffsetX = 1.0 + overshootX,
                      shoulderOffsetY = 1.0 + overshootY,
                      shoulderScaleX  = -1.0,
                      shoulderScaleY  = -1.0,
                      shoulderLnA     = tmp.x,
                      shoulderB       = tmp.y;

                // Normalize (correct for overshooting)
                float scale = evalCurveSegment(1.0,
                                               shoulderOffsetX, shoulderOffsetY,
                                               shoulderScaleX, shoulderScaleY,
                                               shoulderLnA, shoulderB);
                float invScale = 1.0 / scale;
                toeOffsetY      *= invScale;
                toeScaleY       *= invScale;
                midOffsetY      *= invScale;
                midScaleY       *= invScale;
                shoulderOffsetY *= invScale;
                shoulderScaleY  *= invScale;

                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve directly on color input
                vec3 Cout;
                for (int i = 0; i < 3; ++i) {
                    float normX = Cin[i] * curveWinv;
                    float res;
                    if (normX < x0) {
                        res = evalCurveSegment(normX,
                                               toeOffsetX, toeOffsetY,
                                               toeScaleX, toeScaleY,
                                               toeLnA, toeB);
                    } else if (normX < x1) {
                        res = evalCurveSegment(normX,
                                               midOffsetX, midOffsetY,
                                               midScaleX, midScaleY,
                                               midLnA, midB);
                    } else {
                        res = evalCurveSegment(normX,
                                               shoulderOffsetX, shoulderOffsetY,
                                               shoulderScaleX, shoulderScaleY,
                                               shoulderLnA, shoulderB);
                    }
                    Cout[i] = res;
                }

                /* Gamma correction is already included in the mapping above
                   and only clamping is applied. */
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"]  = Parameter(2.2f, 0.f,   10.f,        "gamma",  "Gamma correction value.");
        parameters["tStr"]   = Parameter(0.5f, 0.f,   1.f,         "tStr",   "Toe strength.");
        parameters["tLen"]   = Parameter(0.5f, 0.f,   1.f,         "tLen",   "Toe length.");
        parameters["sStr"]   = Parameter(2.f,  0.f,   10.f,        "sStr",   "Shoulder strength.");
        parameters["sLen"]   = Parameter(0.5f, 1e-5f, 1.f - 1e-5f, "sLen",   "Shoulder length.");
        parameters["sAngle"] = Parameter(1.f,  0.f,   1.f,         "sAngle", "Shoulder angle.");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        auto asSlopeIntercept = [](float x0, float x1, float y0, float y1) {
            float m, b;
            float dy = (y1 - y0),
                  dx = (x1 - x0);
            if (dx == 0.f) {
                m = 1.f;
            } else {
                m = dy / dx;
            }
            b = y0 - x0 * m;
            return std::make_pair(m, b);
        };

        auto evalDerivativeLinearGamma = [](float m, float b, float g, float x) {
            return g * m * std::pow(m * x + b, g - 1.f);
        };

        auto solveAB = [](float x0, float y0, float m) {
            float B = (m * x0) / y0,
                  lnA = log(y0) - B * log(x0);
            return std::make_pair(lnA, B);
        };

        auto evalCurveSegment = [](float x, float offsetX, float offsetY, float scaleX, float scaleY, float lnA, float B) {
            float x0 = (x - offsetX) * scaleX,
                  y0 = 0.0;
            if (x0 > 0.0) {
                y0 = std::exp(lnA + B * log(x0));
            }
            return y0 * scaleY + offsetY;
        };

        // Fetch parameters
        float gamma  = parameters.at("gamma").value,
              tStr   = parameters.at("tStr").value,
              tLen   = parameters.at("tLen").value,
              sStr   = parameters.at("sStr").value,
              sLen   = parameters.at("sLen").value,
              sAngle = parameters.at("sAngle").value;

        // Convert from "user" to "direct" parameters
        float tLen_      = std::pow(tLen, 2.2f),
              x0         = 0.5f * tLen_,
              y0         = (1.f - tStr) * x0,
              remainingY = 1.f - y0,
              initialW   = x0 + remainingY,
              y1Offset   = (1.f - sLen) * remainingY,
              x1         = x0 + y1Offset,
              y1         = y0 + y1Offset,
              extraW     = std::pow(2.f, sStr) - 1.f,
              W          = initialW + extraW,
              overshootX = (2.f * W) * sAngle * sStr,
              overshootY = 0.5f * sAngle * sStr,
              invGamma   = 1.f / gamma;

        // Precompute information for all three segments (mid, toe, shoulder)
        float curveWinv = 1.f / W;
        x0 /= W;
        x1 /= W;
        overshootX /= W;

        auto tmp = asSlopeIntercept(x0, x1, y0, y1);
        float m = tmp.first,
              b = tmp.second,
              g = invGamma;

        float midOffsetX = -(b / m),
              midOffsetY = 0.f,
              midScaleX  = 1.f,
              midScaleY  = 1.f,
              midLnA = g * log(m),
              midB = g;

        float toeM      = evalDerivativeLinearGamma(m, b, g, x0),
              shoulderM = evalDerivativeLinearGamma(m, b, g, x1);

        y0 = std::max(1e-5f, std::pow(y0, invGamma));
        y1 = std::max(1e-5f, std::pow(y1, invGamma));
        overshootY = std::pow(1.f + overshootY, invGamma) - 1.f;

        tmp = solveAB(x0, y0, toeM);

        float toeOffsetX = 0.f,
              toeOffsetY = 0.f,
              toeScaleX  = 1.f,
              toeScaleY  = 1.f,
              toeLnA     = tmp.first,
              toeB       = tmp.second;

        float shoulderX0 = (1.f + overshootX) - x1,
              shoulderY0 = (1.f + overshootY) - y1;
        tmp = solveAB(shoulderX0, shoulderY0, shoulderM);

        float shoulderOffsetX =  1.f + overshootX,
              shoulderOffsetY =  1.f + overshootY,
              shoulderScaleX  = -1.f,
              shoulderScaleY  = -1.f,
              shoulderLnA     = tmp.first,
              shoulderB       = tmp.second;

        // Normalize (correct for overshooting)
        float scale = evalCurveSegment(1.f,
                                       shoulderOffsetX, shoulderOffsetY,
                                       shoulderScaleX, shoulderScaleY,
                                       shoulderLnA, shoulderB);
        float invScale = 1.f / scale;
        toeOffsetY      *= invScale;
        toeScaleY       *= invScale;
        midOffsetY      *= invScale;
        midScaleY       *= invScale;
        shoulderOffsetY *= invScale;
        shoulderScaleY  *= invScale;

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve directly on color input
        Color3f Cout;
        for (int i = 0; i < 3; ++i) {
            float normX = Cin[i] * curveWinv;
            float res;
            if (normX < x0) {
                res = evalCurveSegment(normX,
                                       toeOffsetX, toeOffsetY,
                                       toeScaleX, toeScaleY,
                                       toeLnA, toeB);
            } else if (normX < x1) {
                res = evalCurveSegment(normX,
                                       midOffsetX, midOffsetY,
                                       midScaleX, midScaleY,
                                       midLnA, midB);
            } else {
                res = evalCurveSegment(normX,
                                       shoulderOffsetX, shoulderOffsetY,
                                       shoulderScaleX, shoulderScaleY,
                                       shoulderLnA, shoulderB);
            }
            Cout[i] = res;
        }

        /* Gamma correction is already included in the mapping above
           and only clamping is applied. */
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(HableUpdatedFilmicOperator, "hable_updated");

} // Namespace tonemapper
