/*
    src/ward.h -- Ward tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class WardOperator : public TonemapOperator {
public:
    WardOperator() : TonemapOperator() {
        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
        parameters["Ldmax"] = Parameter(100.f, 0.f, 200.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");

        name = "Ward";
        description = "Ward Mapping\n\nProposed in \"A contrast-based scalefactor for luminance display\" by Ward 1994.";

        shader->init(
            "Ward",

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
            uniform float Lwa;
            uniform float Ldmax;
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
                float Lda = Ldmax / 2.0;
                float m = pow((1.219 + pow(Lda, 0.4)) / (1.219 + pow(Lwa * exposure, 0.4)), 2.5);
                vec4 color = exposure * texture(source, uv);
                float L = getLuminance(color);
                float Ld = m * L / Ldmax;
                color = adjustColor(color, L, Ld);
                color = clampedValue(color);
                out_color = gammaCorrect(color);
            }
            )glsl"
        );
    }

    virtual void setParameters(const Image *image) override {
        parameters["Lwa"] = Parameter(image->getLogAverageLuminance(), "Lwa");
    };

    void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
        const nanogui::Vector2i &size = image->getSize();
        *progress = 0.f;
        float delta = 1.f / (size.x() * size.y());

        float gamma = parameters.at("Gamma").value;
        float Lwa = parameters.at("Lwa").value;
        float Ldmax = parameters.at("Ldmax").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                float Lw = color.getLuminance();
                float Ld = map(Lw, exposure, Lwa, Ldmax);
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
        float Lwa = parameters.at("Lwa").value;
        float Ldmax = parameters.at("Ldmax").value;

        value = map(value, 1.f, Lwa, Ldmax);
        value = clamp(value, 0.f, 1.f);
        value = std::pow(value, 1.f / gamma);
        return value;
    }

protected:
    float map(float Lw, float exposure, float Lwa, float Ldmax) const {
        float Lda = Ldmax / 2.f;
        float m = std::pow((1.219f + std::pow(Lda, 0.4f)) / (1.219f + std::pow(Lwa * exposure, 0.4f)), 2.5f);

        float L = exposure * Lw;
        float Ld = m * L / Ldmax;
        return Ld;
    }
};