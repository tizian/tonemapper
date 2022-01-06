/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class TumblinRushmeierOperator : public TonemapOperator {
public:
    TumblinRushmeierOperator() : TonemapOperator() {
        name = "Tumblin Rushmeier";
        description = R"(Mapping proposed in "Tone Reproduction for Realistic
            Images" by Tumblin and Rushmeier 1993.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float Lavg;
            uniform float Ldmax;
            uniform float Cmax;

            float luminance(vec3 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            float log10(float x) {
                return log(x) / log(10.0);
            }

            void main() {
                // Fetch color and convert to luminance
                vec3 Cin = exposure * texture(source, uv).rgb;
                float Lin = luminance(Cin);

                // Apply exposure scale to parameters
                float Lavg_ = exposure * Lavg;

                // Apply tonemapping curve to luminance
                float logLrw   =  log10(Lavg_) + 0.84,
                      alphaRw  =  0.4 * logLrw + 2.92,
                      betaRw   = -0.4 * logLrw * logLrw - 2.584 * logLrw + 2.0208,
                      Lwd      =  Ldmax / sqrt(Cmax),
                      logLd    =  log10(Lwd) + 0.84,
                      alphaD   =  0.4 * logLd + 2.92,
                      betaD    = -0.4 * logLd * logLd - 2.584 * logLd + 2.0208,
                      Lout     =  pow(Lin, alphaRw / alphaD) / Ldmax * pow(10.0, (betaRw - betaD) / alphaD) - (1.0 / Cmax);

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
        parameters["Cmax"]  = Parameter(36.f, 1.f, 100.f, "Cmax",  "Maximum contrast ratio betwen on-screen luminances.");
    }

    void preprocess(const Image *image) override {
        // World adaptation level, approximated by the log average luminance over the image.
        parameters["Lavg"] = Parameter(image->getMeanLuminance(), "Lavg");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma = parameters.at("gamma").value,
              Lavg  = parameters.at("Lavg").value,
              Ldmax = parameters.at("Ldmax").value,
              Cmax  = parameters.at("Cmax").value;

        // Fetch color and convert to luminance
        Color3f Cin = exposure * color;
        float Lin = luminance(Cin);

        // Apply exposure scale to parameters
        Lavg *= exposure;

        // Apply tonemapping curve to luminance
        float logLrw   = std::log10(Lavg) + 0.84f,
              alphaRw  =  0.4f * logLrw + 2.92f,
              betaRw   = -0.4f * logLrw * logLrw - 2.584f * logLrw + 2.0208f,
              Lwd      = Ldmax / std::sqrt(Cmax),
              logLd    = std::log10(Lwd) + 0.84f,
              alphaD   =  0.4f * logLd + 2.92f,
              betaD    = -0.4f * logLd * logLd - 2.584f * logLd + 2.0208f,
              Lout     = std::pow(Lin, alphaRw / alphaD) / Ldmax * std::pow(10.f, (betaRw - betaD) / alphaD) - (1.f / Cmax);

        // Treat color by preserving color ratios [Schlick 1994].
        Color3f Cout = Cin / Lin * Lout;

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(TumblinRushmeierOperator, "tumblin_rushmeier");

} // Namespace tonemapper
