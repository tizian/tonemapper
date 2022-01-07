/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class AcesGuyFilmicOperator : public TonemapOperator {
public:
    AcesGuyFilmicOperator() : TonemapOperator() {
        name = "Guy ACES";
        description = R"(Curve from "Unreal 3" adapted by to close to the ACES
            curve by Romain Guy)";

        /* See also the following shadertoy by Romain Guy:
           https://www.shadertoy.com/view/llXyWr */
        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve directly on color input
                vec3 Cout = Cin / (Cin + 0.155) * 1.019;

                /* Gamma correction is already included in the mapping above
                   and only clamping is applied. */
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve directly on color input
        Color3f Cout = Cin / (Cin + 0.155f) * 1.019f;

        /* Gamma correction is already included in the mapping above
           and only clamping is applied. */
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(AcesGuyFilmicOperator, "aces_guy");

} // Namespace tonemapper
