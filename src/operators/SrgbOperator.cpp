#include <Tonemap.h>

namespace tonemapper {

class SrgbOperator : public TonemapOperator {
public:
    SrgbOperator() : TonemapOperator() {
        name = "sRGB";
        description = "Convert into sRGB color space.";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;

            float toSRGB(float value) {
                if (value < 0.0031308)
                    return 12.92 * value;
                return 1.055 * pow(value, 0.41666) - 0.055;
            }

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                color = vec4(toSRGB(color.r), toSRGB(color.g), toSRGB(color.b), 1.0);
                out_color = clampedValue(color);
            }
        )glsl";
    }

    virtual Color3f map(const Color3f &c) const {
        Color3f color = c;
        for (size_t i = 0; i < 3; ++i) {
            if (color[i] < 0.0031308f) {
                color[i] = 12.92f * color[i];
            } else {
                color[i] = 1.055f * std::pow(color[i], 0.41666f) - 0.055f;
            }
        }
        return color;
    }
};

REGISTER_OPERATOR(SrgbOperator, "srgb");

} // Namespace tonemapper
