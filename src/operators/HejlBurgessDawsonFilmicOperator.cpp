/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class HejlBurgessDawsonFilmicOperator : public TonemapOperator {
public:
    HejlBurgessDawsonFilmicOperator() : TonemapOperator() {
        name = "Hejl Burgess-Dawson Filmic";
        description = R"(Analytical approximation of a Kodak film curve by Jim
            Hejl and Richard Burgess-Dawson. See the "Filmic Tonemapping for
            Real-time Rendering" SIGGRAPH 2010 course by Haarm-Pieter Duiker.)";

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
                vec3 x    = max(vec3(0.0), Cin - 0.004),
                     Cout = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);

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
        Color3f x    = max(Color3f(0.f), Cin - 0.004f),
                Cout = (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);

        /* Gamma correction is already included in the mapping above
           and only clamping is applied. */
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(HejlBurgessDawsonFilmicOperator, "hejl_burgess_dawson");

} // Namespace tonemapper
