/*
    Copyright (c) 2022 Tizian Zeltner

    tonemapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class DayFilmicOperator : public TonemapOperator {
public:
    DayFilmicOperator() : TonemapOperator() {
        name = "Day Filmic";
        description = R"(Filmic curve by Mike Day, described in his document "An
            efficient and user-friendly tone mapping operator". Also known as
            the "Insomniac curve".)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float w;
            uniform float b;
            uniform float t;
            uniform float s;
            uniform float c;
            uniform float Lavg;

            float curve(float x, float k) {
                if (x < c) {
                    return k * (1.0 - t) * (x - b) / (c - (1.0 - t) * b - t * x);
                } else {
                    return (1.0 - k) * (x - c) / (s * x + (1.0 - s) * w - c) + k;
                }
            }

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve directly on color input
                vec3 Cout = Cin / Lavg;
                float k = (1.0 - t) * (c - b) / ((1.0 - s) * (w - c) + (1.0 - t) * (c - b));
                Cout = vec3(curve(Cout.r, k), curve(Cout.g, k), curve(Cout.b, k));

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value.");
        parameters["w"]     = Parameter(10.f, 0.f, 20.f, "w",     "White point. Smallest value that is mapped to 1.");
        parameters["b"]     = Parameter(0.1f, 0.f, 2.f,  "b",     "Black point. Largest value that is mapped to 0.");
        parameters["t"]     = Parameter(0.7f, 0.f, 1.f,  "t",     "Toe strength. Amount of blending between a straight-line curve and a purely asymptotic curve for the toe.");
        parameters["s"]     = Parameter(0.8f, 0.f, 1.f,  "s",     "Shoulder strength. Amount of blending between a straight-line curve and a purely asymptotic curve for the shoulder.");
        parameters["c"]     = Parameter(2.f,  0.f, 10.f, "c",     "Cross-over point. Point where the toe and shoulder are pieced together into a single curve.");
    }

    void preprocess(const Image *image) override {
        parameters["Lavg"] = Parameter(image->getMeanLuminance(), "Lavg");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma = parameters.at("gamma").value,
              w     = parameters.at("w").value,
              b     = parameters.at("b").value,
              t     = parameters.at("t").value,
              s     = parameters.at("s").value,
              c     = parameters.at("c").value,
              Lavg  = parameters.at("Lavg").value;

        auto curve = [w, b, t, s, c](float x, float k) {
            if (x < c) {
                return k * (1.f - t) * (x - b) / (c - (1.f - t) * b - t * x);
            } else {
                return (1.f - k) * (x - c) / (s * x + (1.f - s) * w - c) + k;
            }
        };

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve directly on color input
        Color3f Cout = Cin / Lavg;
        float k = (1.f - t) * (c - b) / ((1.f - s) * (w - c) + (1.f - t) * (c - b));
        Cout = Color3f(curve(Cout.r(), k), curve(Cout.g(), k), curve(Cout.b(), k));

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(DayFilmicOperator, "day");

} // Namespace tonemapper
