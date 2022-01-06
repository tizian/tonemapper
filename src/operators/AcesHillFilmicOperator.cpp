/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class AcesHillFilmicOperator : public TonemapOperator {
public:
    AcesHillFilmicOperator() : TonemapOperator() {
        name = "Hill ACES";
        description = R"(ACES curve fit by Stephen Hill.)";

        // See https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;

            vec3 mulInput(vec3 color) {
                float a = 0.59719 * color.r + 0.35458 * color.g + 0.04823 * color.b,
                      b = 0.07600 * color.r + 0.90834 * color.g + 0.01566 * color.b,
                      c = 0.02840 * color.r + 0.13383 * color.g + 0.83777 * color.b;
                return vec3(a, b, c);
            }

            vec3 mulOutput(vec3 color) {
                float a =  1.60475 * color.r - 0.53108 * color.g - 0.07367 * color.b,
                      b = -0.10208 * color.r + 1.10813 * color.g - 0.00605 * color.b,
                      c = -0.00327 * color.r - 0.07276 * color.g + 1.07602 * color.b;
                return vec3(a, b, c);
            }

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve directly on color input
                Cin = mulInput(Cin);
                vec3 a    = Cin * (Cin + 0.0245786) - 0.000090537,
                     b    = Cin * (0.983729 * Cin + 0.4329510) + 0.238081,
                     Cout = a / b;
                Cout = mulOutput(Cout);

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value.");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        auto mulInput = [](const Color3f &color) {
            float a = 0.59719f * color.r() + 0.35458f * color.g() + 0.04823f * color.b(),
                  b = 0.07600f * color.r() + 0.90834f * color.g() + 0.01566f * color.b(),
                  c = 0.02840f * color.r() + 0.13383f * color.g() + 0.83777f * color.b();
            return Color3f(a, b, c);
        };

        auto mulOutput = [](const Color3f &color) {
            float a =  1.60475f * color.r() - 0.53108f * color.g() - 0.07367f * color.b(),
                  b = -0.10208f * color.r() + 1.10813f * color.g() - 0.00605f * color.b(),
                  c = -0.00327f * color.r() - 0.07276f * color.g() + 1.07602f * color.b();
            return Color3f(a, b, c);
        };

        // Fetch parameters
        float gamma = parameters.at("gamma").value;

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve directly on color input
        Cin = mulInput(Cin);
        Color3f a    = Cin * (Cin + 0.0245786f) - 0.000090537f,
                b    = Cin * (0.983729f * Cin + 0.4329510f) + 0.238081f,
                Cout = a / b;
        Cout = mulOutput(Cout);

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(AcesHillFilmicOperator, "aces_hill");

} // Namespace tonemapper
