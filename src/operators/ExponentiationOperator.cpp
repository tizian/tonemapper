#include <Tonemap.h>

namespace tonemapper {

class ExponentiationOperator : public TonemapOperator {
public:
    ExponentiationOperator() : TonemapOperator() {
        name = "Exponentiation";
        description = R"(Exponentiation mapping as discussed in "Quantization
            Techniques for Visualization of High Dynamic Range Pictures" by
            Schlick 1994.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float p;
            uniform float Lmax;

            float luminance(vec3 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            void main() {
                // Fetch color and convert to luminance
                vec3 Cin = exposure * texture(source, uv).rgb;
                float Lin = luminance(Cin);

                // Apply exposure scale to parameters
                float Lmax_ = exposure * Lmax;

                // Apply tonemapping curve to luminance
                float Lout = pow(Lin / Lmax_, p);

                // Treat color by preserving color ratios [Schlick 1994].
                vec3 Cout = Cin / Lin * Lout;

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value.");
        parameters["p"]     = Parameter(0.5f, 0.f, 1.f,  "p",     "Curve exponent parameter");
    }

    void preprocess(const Image *image) override {
        parameters["Lmax"] = Parameter(image->getMaximumLuminance(), "Lmax");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma = parameters.at("gamma").value,
              p     = parameters.at("p").value,
              Lmax  = parameters.at("Lmax").value;

        // Fetch color and convert to luminance
        Color3f Cin = exposure * color;
        float Lin = luminance(Cin);

        // Apply exposure scale to parameters
        Lmax *= exposure;

        // Apply tonemapping curve to luminance
        float Lout = std::pow(Lin / Lmax, p);

        // Treat color by preserving color ratios [Schlick 1994].
        Color3f Cout = Cin / Lin * Lout;

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(ExponentiationOperator, "exponentiation");

} // Namespace tonemapper
