/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class DurandDorseyOperator : public TonemapOperator {
public:
    DurandDorseyOperator() : TonemapOperator() {
        name = "Durand Dorsey";
        description = R"(Mapping proposed in "Interactive Tone Mapping" by
            Durand and Dorsey 2000, which is a modified version of the operator
            by Ferwerda et al. 1996.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float Ldmax;
            uniform float Lwap;
            uniform float Lwas;
            uniform float k;

            float luminance(vec3 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            float luminanceRods(vec3 color) {
                /* From "A Multiscale Model of Adaptation and Spatial Vision for
                   Realistic Image Display" by Pattanaik et al. 1998 */
                float X = 0.412453 * color.r + 0.357580 * color.g + 0.180423 * color.b,
                      Y = 0.212671 * color.r + 0.715160 * color.g + 0.072169 * color.b,
                      Z = 0.019334 * color.r + 0.119193 * color.g + 0.950227 * color.b;
                return -0.702 * X + 1.039 * Y + 0.433 * Z;
            }

            float log10(float x) {
                return log(x) / log(10.0);
            }

            float tp(float La) {
                // Photopic threshold (for cones)
                float logLa = log10(La);
                float result = 0.0;
                if (logLa <= -2.6) {
                    result = -0.72;
                } else if (logLa >= 1.9) {
                    result = logLa - 1.255;
                } else {
                    result = pow(0.249 * logLa + 0.65, 2.7) - 0.72;
                }
                return pow(10.0, result);
            }

            float ts(float La) {
                // Scotopic threshold (for rods)
                float logLa = log10(La);
                float result = 0.0;
                if (logLa <= -3.94) {
                    result = -2.86;
                } else if (logLa >= -1.44) {
                    result = logLa - 0.395;
                } else {
                    result = pow(0.405 * logLa + 1.6, 2.18) - 2.86;
                }
                return pow(10.0, result);
            }

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply tonemapping curve directly to RGB (cone) and rod signal
                float Lda = 0.5 * Ldmax,
                      Ls = luminanceRods(Cin),
                      mp = tp(Lda) / tp(Lwap),
                      ms = tp(Lda) / ts(Lwas);
                vec3 blueShift = vec3(0.105, 0.97, 1.27);
                vec3 Cout = (mp * Cin + blueShift * k * ms * vec3(Ls)) / Ldmax;

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f, 0.f, 10.f,  "gamma", "Gamma correction value.");
        parameters["Ldmax"] = Parameter(80.f, 1.f, 150.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");
    }

    void preprocess(const Image *image) override {
        // World adaptation level for cones, approximated by half the maximum luminance
        float Lwap = 0.5f*image->getMaximumLuminance();
        parameters["Lwap"] = Parameter(Lwap, "Lwap");
        // World adaptation level for rods, approximated by half the maximum rod signal
        float Lwas = 0.5f*luminanceRods(image->getMaximum());
        parameters["Lwas"] = Parameter(Lwas, "Lwas");

        float sigma = 100.f,
              k     = (sigma - 0.25f * Lwas) / (sigma + Lwas);
        k = std::clamp(k * k, 0.f, 1.f);
        parameters["k"] = Parameter(k, 0.f, 1.f, "k", "Blend between photopic and scotopic world adaption to account for mesopic range in between.");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        auto tp = [](float La) {
            // Photopic threshold (for cones)
            float logLa = std::log10(La);
            float result = 0.f;
            if (logLa <= -2.6f) {
                result = -0.72f;
            } else if (logLa >= 1.9f) {
                result = logLa - 1.255f;
            } else {
                result = std::pow(0.249f * logLa + 0.65f, 2.7f) - 0.72f;
            }
            return std::pow(10.f, result);
        };

        auto ts = [](float La) {
            // Scotopic threshold (for rods)
            float logLa = std::log10(La);
            float result = 0.f;
            if (logLa <= -3.94f) {
                result = -2.86f;
            } else if (logLa >= -1.44f) {
                result = logLa - 0.395f;
            } else {
                result = std::pow(0.405f * logLa + 1.6f, 2.18f) - 2.86f;
            }
            return std::pow(10.f, result);
        };

        // Fetch parameters
        float gamma = parameters.at("gamma").value,
              Ldmax = parameters.at("Ldmax").value,
              Lwap  = parameters.at("Lwap").value,
              Lwas  = parameters.at("Lwas").value,
              k     = parameters.at("k").value;

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply tonemapping curve directly to RGB (cone) and rod signal
        float Lda = 0.5f * Ldmax,
              Ls = luminanceRods(Cin),
              mp = tp(Lda) / tp(Lwap),
              ms = tp(Lda) / ts(Lwas);
        Color3f blueShift = Color3f(0.105f, 0.97f, 1.27f);
        Color3f Cout = (mp * Cin + blueShift * k * ms * Color3f(Ls)) / Ldmax;

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(DurandDorseyOperator, "durand_dorsey");

} // Namespace tonemapper
