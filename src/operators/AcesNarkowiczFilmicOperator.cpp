/*
    Copyright (c) 2022 Tizian Zeltner

    tonemapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class AcesNarkowiczFilmicOperator : public TonemapOperator {
public:
    AcesNarkowiczFilmicOperator() : TonemapOperator() {
        name = "Narkowicz ACES";
        description = R"(ACES curve fit by Krzysztof Narkowicz. See his blog
            post "ACES Filmic Tone Mapping Curve".)";

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

                // Apply curve directly on color input
                float a = 2.51,
                      b = 0.03,
                      c = 2.43,
                      d = 0.59,
                      e = 0.14;
                Cin *= 0.6;
                vec3 Cout = (Cin * (a * Cin + b)) / (Cin * (c * Cin + d) + e);

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
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

        // Apply curve directly on color input
        float a = 2.51f,
              b = 0.03f,
              c = 2.43f,
              d = 0.59f,
              e = 0.14f;
        Cin *= 0.6f;
        Color3f Cout = (Cin * (a * Cin + b)) / (Cin * (c * Cin + d) + e);

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(AcesNarkowiczFilmicOperator, "aces_narkowicz");

} // Namespace tonemapper
