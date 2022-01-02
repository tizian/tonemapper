#include <Tonemap.h>

namespace tonemapper {

class DragoOperator : public TonemapOperator {
public:
    DragoOperator() : TonemapOperator() {
        name = "Drago";
        description = R"(Mapping proposed in "Adaptive Logarithmic Mapping For
            Displaying High Contrast Scenes" by Drago et al. 2003.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float Ldmax;
            uniform float Lwa;
            uniform float Lmax;
            uniform float b;
            uniform float slope;
            uniform float start;

            float luminance(vec3 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            float log10(float x) {
                return log(x) / log(10.0);
            }

            float customGamma(float C) {
                if (C <= start) {
                    return slope * C;
                } else {
                    return pow(1.099 * C, 0.9 / gamma) - 0.099;
                }
            }

            void main() {
                // Fetch color and convert to luminance
                vec3 Cin = exposure * texture(source, uv).rgb;
                float Lin = luminance(Cin);

                // Apply exposure scale to parameters
                float Lmax_ = exposure * Lmax;

                // Bias the world adaptation and scale other parameters accordingly
                float LwaP  = Lwa / pow(1.0 + b - 0.85, 5.0),
                      LmaxP = Lmax_ / LwaP,
                      LinP  = Lin / LwaP;

                // Apply tonemapping curve to luminance
                float exponent = log(b) / log(0.5),
                      c1       = (0.01 * Ldmax) / log10(1.0 + LmaxP),
                      c2       = log(1.0 + LinP) / log(2.0 + 8.0 * pow(LinP / LmaxP, exponent)),
                      Lout     = c1 * c2;

                // Treat color by preserving color ratios [Schlick 1994].
                vec3 Cout = Cin / Lin * Lout;

                // Apply a custom gamma curve and clamp
                Cout = vec3(customGamma(Cout.r), customGamma(Cout.g), customGamma(Cout.b));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f,   0.f, 10.f,  "gamma", "Gamma correction value.");
        parameters["Ldmax"] = Parameter(80.f,   1.f, 150.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");
        parameters["b"]     = Parameter(0.85f,  0.f, 1.f,   "b",     "Bias function parameter");
        parameters["slope"] = Parameter(4.5f,   0.f, 10.f,  "slope", "Elevation ratio of the line passing by the origin and tangent to the curve (for custom gamma correction).");
        parameters["start"] = Parameter(0.018f, 0.f, 1.f,   "start", "Abscissa at the point of tangency (for custom gamma correction).");
    }

    void preprocess(const Image *image) override {
        // World adaptation level, approximated by the log average luminance over the image.
        parameters["Lwa"]  = Parameter(image->getLogMeanLuminance(), "Lwa");
        parameters["Lmax"] = Parameter(image->getMaximumLuminance(), "Lmax");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma = parameters.at("gamma").value,
              Ldmax = parameters.at("Ldmax").value,
              Lwa   = parameters.at("Lwa").value,
              Lmax  = parameters.at("Lmax").value,
              b     = parameters.at("b").value,
              slope = parameters.at("slope").value,
              start = parameters.at("start").value;

        auto customGamma = [start, slope, gamma](float C) {
            if (C <= start) {
                return slope * C;
            } else {
                return std::pow(1.099f * C, 0.9f / gamma) - 0.099f;
            }
        };

        // Fetch color and convert to luminance
        Color3f Cin = exposure * color;
        float Lin = luminance(Cin);

        // Apply exposure scale to parameters
        Lmax *= exposure;

        // Bias the world adaptation and scale other parameters accordingly
        float LwaP  = Lwa / std::pow(1.f + b - 0.85f, 5.f),
              LmaxP = Lmax / LwaP,
              LinP  = Lin / LwaP;

        // Apply tonemapping curve to luminance
        float exponent = std::log(b) / std::log(0.5f),
              c1       = (0.01f * Ldmax) / std::log10(1.f + LmaxP),
              c2       = std::log(1.f + LinP) / std::log(2.f + 8.f * std::pow(LinP / LmaxP, exponent)),
              Lout     = c1 * c2;

        // Treat color by preserving color ratios [Schlick 1994].
        Color3f Cout = Cin / Lin * Lout;

        // Apply gamma curve and clamp
        Cout = Color3f(customGamma(Cout.r()), customGamma(Cout.g()), customGamma(Cout.b()));
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(DragoOperator, "drago");

} // Namespace tonemapper
