#include <Tonemap.h>

namespace tonemapper {

class LinearOperator : public TonemapOperator {
public:
    LinearOperator() : TonemapOperator() {
        name = "Linear";
        description = "Do not apply any processing apart from the most basic gamma correction.";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            vec4 gammaCorrect(vec4 color) {
                return pow(color, vec4(1.0/gamma));
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                color = clampedValue(color);
                out_color = gammaCorrect(color);
            }
        )glsl";

        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value.");
    }

    virtual Color3f map(const Color3f &c) const {
        float gamma = parameters.at("Gamma").value;
        return pow(c, 1.f / gamma);
    }
};

REGISTER_OPERATOR(LinearOperator, "linear");

} // Namespace tonemapper
