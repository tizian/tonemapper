/*
    Copyright (c) 2022 Tizian Zeltner

    tonemapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class GammaOperator : public TonemapOperator {
public:
    GammaOperator() : TonemapOperator() {
        name = "Gamma";
        description = R"(Do not apply any processing apart from the most basic
            gamma correction.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply gamma curve and clamp
                vec3 Cout = pow(Cin, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value.");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma = parameters.at("gamma").value;

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply gamma curve and clamp
        Color3f Cout = pow(Cin, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(GammaOperator, "gamma");

} // Namespace tonemapper
