#include <Tonemap.h>

namespace tonemapper {

class HableFilmicOperator : public TonemapOperator {
public:
    HableFilmicOperator() : TonemapOperator() {
        name = "Hable Filmic";
        description = R"(Filmic curve by John Hable, see the "Filmic Tonemapping
            for Real-time Rendering" SIGGRAPH 2010 course. Also known as the
            "Uncharted 2 curve".)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float A;
            uniform float B;
            uniform float C;
            uniform float D;
            uniform float E;
            uniform float F;
            uniform float W;

            vec3 curve(vec3 x) {
                return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
            }

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve directly on color input
                float exposureBias = 2.0;
                vec3 Cout = exposureBias * curve(Cin) / curve(vec3(W));

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f,  0.f, 10.f, "gamma", "Gamma correction value.");
        parameters["A"]     = Parameter(0.15,  0.f, 1.f,  "A",     "Shoulder strength.");
        parameters["B"]     = Parameter(0.5f,  0.f, 1.f,  "B",     "Linear strength.");
        parameters["C"]     = Parameter(0.1f,  0.f, 1.f,  "C",     "Linear angle.");
        parameters["D"]     = Parameter(0.2f,  0.f, 1.f,  "D",     "Toe strength.");
        parameters["E"]     = Parameter(0.02f, 0.f, 1.f,  "E",     "Toe numerator.");
        parameters["F"]     = Parameter(0.3f,  0.f, 1.f,  "F",     "Toe denominator.");
        parameters["W"]     = Parameter(11.2f, 0.f, 20.f, "W",     "Linear white point value.");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma = parameters.at("gamma").value,
              A     = parameters.at("A").value,
              B     = parameters.at("B").value,
              C     = parameters.at("C").value,
              D     = parameters.at("D").value,
              E     = parameters.at("E").value,
              F     = parameters.at("F").value,
              W     = parameters.at("W").value;

        auto curve = [A, B, C, D, E, F](const Color3f &x) {
            return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
        };

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve directly on color input
        float exposureBias = 2.0;
        Color3f Cout = exposureBias * curve(Cin) / curve(Color3f(W));

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(HableFilmicOperator, "hable");

} // Namespace tonemapper
