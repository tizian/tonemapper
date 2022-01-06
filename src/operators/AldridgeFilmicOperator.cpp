/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class AldridgeFilmicOperator : public TonemapOperator {
public:
    AldridgeFilmicOperator() : TonemapOperator() {
        name = "Aldridge Filmic";
        description = R"(Variation of the Hejl and Burgess-Dawson filmic curve
            done by Graham Aldridge, see his blog post about "Approximating Film
            with Tonemapping".)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float cutoff;

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve directly on color input
                vec3 tmp  = vec3(2.0 * cutoff),
                     x    = Cin + (tmp - Cin) * clamp(tmp - Cin, 0.0, 1.0) * (0.25 / cutoff) - cutoff,
                     Cout = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);

                /* Gamma correction is already included in the mapping above
                   and only clamping is applied. */
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["cutoff"] = Parameter(0.025f, 0.f, 0.5f, "cutoff", "Transition into compressed blacks.");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float cutoff = parameters.at("cutoff").value;

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve directly on color input
        Color3f tmp  = Color3f(2.f * cutoff),
                x    = Cin + (tmp - Cin) * clamp(tmp - Cin, 0.f, 1.f) * (0.25f / cutoff) - cutoff,
                Cout = (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);

        /* Gamma correction is already included in the mapping above
           and only clamping is applied. */
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(AldridgeFilmicOperator, "aldridge");

} // Namespace tonemapper
