/*
    src/exponentiation.h -- Exponentiation tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class ExponentiationOperator : public TonemapOperator {
public:
    ExponentiationOperator() : TonemapOperator() {
        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
        parameters["p"] = Parameter(0.5f, 0.f, 1.f, "p", "Curve exponent parameter");

        name = "Exponentiation";
        description = "Exponentiation Mapping\n\nDiscussed in \"Quantization Techniques for Visualization of High Dynamic Range Pictures\" by Schlick 1994.";

        shader->init(
            "Exponentiation",

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
            uniform float Lmax;
            uniform float p;
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            vec4 gammaCorrectPlus(vec4 color) {
                return pow(color, vec4(p/gamma));
            }

            float getLuminance(vec4 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            vec4 adjustColor(vec4 color, float L, float Ld) {
                return Ld * color / L;
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                float L = getLuminance(color);
                float Ld = L / (exposure * Lmax);
                color = adjustColor(color, L, Ld);
                color = clampedValue(color);
                out_color = gammaCorrectPlus(color);
            }
            )glsl"
        );
    }

    virtual void setParameters(const Image *image) override {
        parameters["Lmax"] = Parameter(image->getMaximumLuminance(), "Lmax");
    };

    void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
        const nanogui::Vector2i &size = image->getSize();
        *progress = 0.f;
        float delta = 1.f / (size.x() * size.y());

        float gamma = parameters.at("Gamma").value;
        float Lmax = parameters.at("Lmax").value;
        float p = parameters.at("p").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                float Lw = color.getLuminance();
                float Ld = map(Lw, exposure, Lmax);
                Color3f c = Ld * color / Lw;
                c = c.clampedValue();
                c = c.gammaCorrect(gamma / p);  // Include p in gamma correction
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
        float Lmax = parameters.at("Lmax").value;
        float p = parameters.at("p").value;

        value = map(value, 1.f, Lmax);
        value = clamp(value, 0.f, 1.f);
        value = std::pow(value, p / gamma); // Include p in gamma correction
        return value;
    }

protected:
    float map(float Lw, float exposure, float Lmax) const {
        float L = exposure * Lw;
        float Ld = L / (exposure * Lmax);
        return Ld;
    }
};