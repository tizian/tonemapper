#include <Tonemap.h>

namespace tonemapper {

class WardOperator : public TonemapOperator {
public:
    WardOperator() : TonemapOperator() {
        name = "Ward";
        description = R"(Mapping proposed in "A contrast-based scalefactor for
            luminance display" by Ward 1994.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float Ldmax;
            uniform float Lwa;

            float luminance(vec3 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            void main() {
                // Fetch color and convert to luminance
                vec3 Cin = exposure * texture(source, uv).rgb;
                float Lin = luminance(Cin);

                // Apply tonemapping curve to luminance
                float numerator   = 1.219 + pow(0.5*Ldmax, 0.4),
                      denominator = 1.219 + pow(Lwa, 0.4),
                      m = pow(numerator / denominator, 2.5),
                      Lout = m / Ldmax * Lin;

                // Treat color by preserving color ratios [Schlick 1994].
                vec3 Cout = Cin / Lin * Lout;

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f, 0.f, 10.f,  "gamma", "Gamma correction value.");
        parameters["Ldmax"] = Parameter(80.f, 1.f, 150.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");
    }

    void preprocess(const Image *image) override {
        // World adaptation level, approximated by the log average luminance over the image.
        parameters["Lwa"] = Parameter(image->getLogMeanLuminance(), "Lwa");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma = parameters.at("gamma").value,
              Ldmax = parameters.at("Ldmax").value,
              Lwa   = parameters.at("Lwa").value;

        // Fetch color and convert to luminance
        Color3f Cin = exposure * color;
        float Lin = luminance(Cin);

        // Apply tonemapping curve to luminance
        float numerator   = 1.219f + std::pow(0.5f*Ldmax, 0.4f),
              denominator = 1.219f + std::pow(Lwa, 0.4f),
              m = std::pow(numerator / denominator, 2.5f),
              Lout = m / Ldmax * Lin;

        // Treat color by preserving color ratios [Schlick 1994].
        Color3f Cout = Cin / Lin * Lout;

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(WardOperator, "ward");

} // Namespace tonemapper
