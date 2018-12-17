/*
    src/tumblin_rushmeier.h -- Tumblin-Rushmeier tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class TumblinRushmeierOperator : public TonemapOperator {
public:
    TumblinRushmeierOperator() : TonemapOperator() {
        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");

        parameters["Ldmax"] = Parameter(86.f, 1.f, 200.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");
        parameters["Cmax"] = Parameter(50.f, 1.f, 500.f, "Cmax", "Maximum contrast ratio between on-screen luminances");

        name = "Tumblin-Rushmeier";
        description = "Tumblin-Rushmeier Mapping\n\nProposed in\"Tone Reproduction for Realistic Images\" by Tumblin and Rushmeier 1993.";

        shader->init(
            "TumblinRushmeier",

            R"glsl(
            #version 330
            in vec2 position;
            out vec2 uv;
            void main() {
                gl_Position = vec4(position.x*2-1, position.y*2-1, 0.0, 1.0);
                uv = vec2(position.x, 1-position.y);
            }
            )glsl",

            R"glsl(
            #version 330
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float Lavg;
            uniform float Ldmax;
            uniform float Cmax;
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            vec4 gammaCorrect(vec4 color) {
                return pow(color, vec4(1.0/gamma));
            }

            float getLuminance(vec4 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            vec4 adjustColor(vec4 color, float L, float Ld) {
                return Ld * color / L;
            }

            void main() {
                float log10Lrw = log(exposure * Lavg)/log(10.0);
                float alpha_rw = 0.4 * log10Lrw + 2.92;
                float beta_rw = -0.4 * log10Lrw*log10Lrw - 2.584 * log10Lrw + 2.0208;
                float log10Ld = log(Ldmax / sqrt(Cmax))/log(10.0);
                float alpha_d = 0.4 * log10Ld + 2.92;
                float beta_d = -0.4 * log10Ld*log10Ld - 2.584 * log10Ld + 2.0208;

                vec4 color = exposure * texture(source, uv);
                float L = getLuminance(color);
                float Ld = pow(L, alpha_rw/alpha_d) / Ldmax * pow(10.0, (beta_rw - beta_d) / alpha_d) - (1.0 / Cmax);
                color = adjustColor(color, L, Ld);
                color = clampedValue(color);
                out_color = gammaCorrect(color);
            }
            )glsl"
        );
    }

    virtual void setParameters(const Image *image) override {
        parameters["Lavg"] = Parameter(image->getAverageLuminance(), "Lavg");
    };

    void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
        const nanogui::Vector2i &size = image->getSize();
        *progress = 0.f;
        float delta = 1.f / (size.x() * size.y());

        float gamma = parameters.at("Gamma").value;
        float Lavg = parameters.at("Lavg").value;
        float Ldmax = parameters.at("Ldmax").value;
        float Cmax = parameters.at("Cmax").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                float Lw = color.getLuminance();
                float Ld = map(Lw, exposure, Lavg, Ldmax, Cmax);
                Color3f c = Ld * color / Lw;
                c = c.clampedValue();
                c = c.gammaCorrect(gamma);
                dst[0] = (uint8_t) (255.f * c.r());
                dst[1] = (uint8_t) (255.f * c.g());
                dst[2] = (uint8_t) (255.f * c.b());
                dst += 3;
                *progress += delta;
            }
        }
    }

    float graph(float value) const override {
        float gamma = parameters.at("Gamma").value;
        float Lavg = parameters.at("Lavg").value;
        float Ldmax = parameters.at("Ldmax").value;
        float Cmax = parameters.at("Cmax").value;

        value = map(value, 1.f, Lavg, Ldmax, Cmax);
        value = clamp(value, 0.f, 1.f);
        value = std::pow(value, 1.f / gamma);
        return value;
    }

protected:
    float map(float Lw, float exposure, float Lavg, float Ldmax, float Cmax) const {
        float log10Lrw = std::log10(exposure * Lavg);
        float alpha_rw = 0.4f * log10Lrw + 2.92f;
        float beta_rw = -0.4f * log10Lrw*log10Lrw - 2.584f * log10Lrw + 2.0208f;
        float log10Ld = std::log10(Ldmax / std::sqrt(Cmax));
        float alpha_d = 0.4f * log10Ld + 2.92f;
        float beta_d = -0.4f * log10Ld*log10Ld - 2.584f * log10Ld + 2.0208f;

        float L = exposure * Lw;
        float Ld = std::pow(L, alpha_rw/alpha_d) / Ldmax * std::pow(10.f, (beta_rw - beta_d) / alpha_d) - (1.f / Cmax);
        return Ld;
    }
};