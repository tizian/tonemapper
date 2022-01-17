/*
    Copyright (c) 2022 Tizian Zeltner

    Operator originally contributed by Nick Porcino.

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class UchimuraFilmicOperator : public TonemapOperator {
public:
    UchimuraFilmicOperator() : TonemapOperator() {
        name = "Uchimura Filmic";
        description = R"(Filmic curve by Hajime Uchimura, described in his CEDEC
            talk "HDR Theory and Practice". Also known as the "Gran Turismo
            curve".)";

        /* See also the following desmos graph translated by Romain Guy:
           https://www.desmos.com/calculator/gslcdxvipg
           and his shadertoy:
           https://www.shadertoy.com/view/llXyWr */
        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float P;
            uniform float a;
            uniform float m;
            uniform float l;
            uniform float c;
            uniform float b;

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve directly on color input
                float l0 = ((P - m) * l) / a,
                      S0 = m + l0,
                      S1 = m + a * l0,
                      C2 = (a * P) / (P - S1),
                      CP = -C2 / P;

                vec3 w0 = 1.0 - smoothstep(vec3(0.0), vec3(m), Cin),
                     w2 = step(vec3(m + l0), Cin),
                     w1 = vec3(1.0) - w0 - w2;

                vec3 T = m * pow(Cin / m, vec3(c)) + b,        // toe
                     L = m + a * (Cin - m),                    // linear
                     S = P - (P - S1) * exp(CP * (Cin - S0));  // shoulder

                vec3 Cout = T * w0 + L * w1 + S * w2;

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f,  0.f,   10.f,  "gamma", "Gamma correction value.");
        parameters["P"]     = Parameter(1.f,   1.f,   100.f, "P",     "Maximum Brightness.");
        parameters["a"]     = Parameter(1.f,   0.f,   5.f,   "a",     "Contrast.");
        parameters["m"]     = Parameter(0.22f, 0.f,   1.f,   "m",     "Linear section start.");
        parameters["l"]     = Parameter(0.4f,  0.01f, 0.99f, "l",     "Linear section length.");
        parameters["c"]     = Parameter(1.33f, 1.f,   3.f,   "c",     "Black tightness shape.");
        parameters["b"]     = Parameter(0.f,   0.f,   1.f,   "b",     "Black tightness offset.");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // // Fetch parameters
        float gamma = parameters.at("gamma").value,
                  P = parameters.at("P").value,
                  a = parameters.at("a").value,
                  m = parameters.at("m").value,
                  l = parameters.at("l").value,
                  c = parameters.at("c").value,
                  b = parameters.at("b").value;

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve directly on color input
        float l0 = ((P - m) * l) / a,
              S0 = m + l0,
              S1 = m + a * l0,
              C2 = (a * P) / (P - S1),
              CP = -C2 / P;

        Color3f w0 = Color3f(1.f) - smoothstep(Color3f(0.f), Color3f(m), Cin),
                w2 = step(Color3f(m + l0), Cin),
                w1 = Color3f(1.f) - w0 - w2;

        Color3f T = m * pow(Cin / m, c) + b,                       // toe
                L = Color3f(m) + a * (Cin - Color3f(m)),           // linear
                S = Color3f(P) - (P - S1) * exp(CP * (Cin - S0));  // shoulder

        Color3f Cout = T * w0 + L * w1 + S * w2;

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(UchimuraFilmicOperator, "uchimura");

} // Namespace tonemapper
