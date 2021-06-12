#include <Tonemap.h>

namespace tonemapper {

class SrgbOperator : public TonemapOperator {
public:
    SrgbOperator() {
        name = "sRGB";
        description = "sRGB operator.";
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
