#include <Tonemap.h>

namespace tonemapper {

class SrgbOperator : public TonemapOperator {
public:
    SrgbOperator() : TonemapOperator() {
        name = "sRGB";
        description = R"(Convert into sRGB color space.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;

            float toSRGB(float value) {
                if (value < 0.0031308) {
                    return 12.92 * value;
                }
                return 1.055 * pow(value, 0.41666) - 0.055;
            }

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply sRGB conversion
                vec3 Cout = vec3(toSRGB(Cin.r), toSRGB(Cin.g), toSRGB(Cin.b));

                /* Gamma correction is already included in the mapping above
                   and only clamping is applied. */
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";
    }

    Color3f map(const Color3f &color, float exposure) const override {
        auto toSRGB = [](float value) {
            if (value < 0.0031308f) {
                return 12.92f * value;
            }
            return 1.055f * std::pow(value, 0.41666f) - 0.055f;
        };

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply sRGB conversion
        Color3f Cout = Color3f(toSRGB(Cin.r()), toSRGB(Cin.g()), toSRGB(Cin.b()));

        /* Gamma correction is already included in the mapping above
           and only clamping is applied. */
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(SrgbOperator, "srgb");

} // Namespace tonemapper
