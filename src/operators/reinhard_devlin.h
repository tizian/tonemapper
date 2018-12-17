/*
    src/reinhard_devlin.h -- Reinhard-Devlin tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class ReinhardDevlinOperator : public TonemapOperator {
public:
    ReinhardDevlinOperator() : TonemapOperator() {
        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
        parameters["m"] = Parameter(0.5f, 0.f, 1.f, "m", "Compression curve adjustment parameter");
        parameters["f"] = Parameter(1.f, 0.f, 1000.f, "f", "Intensity adjustment parameter");
        parameters["c"] = Parameter(0.f, 0.f, 1.f, "c", "Chromatic adaptation\nBlend between color channels and luminance.");
        parameters["a"] = Parameter(1.f, 0.f, 1.f, "a", "Light adaptation\nBlend between pixel intensity and average scene intensity.");

        name = "Reinhard-Devlin";
        description = "Reinhard-Devlin Mapping\n\nPropsed in \"Dynamic Range Reduction Inspired by Photoreceptor Physiology\" by Reinhard and Devlin 2005.";

        shader->init(
            "ReinhardDevlin",

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
            uniform float m;
            uniform float f;
            uniform float c;
            uniform float a;
            uniform float Iav_r;
            uniform float Iav_g;
            uniform float Iav_b;
            uniform float Lav;
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

            float sigmaIa(float Ia, float Iav_a, float L) {
                float Ia_local = c * Ia + (1.0 - c) * L;
                float Ia_global = c * Iav_a + (1.0 - c) * exposure * Lav;
                float result = a * Ia_local + (1.0 - a) * Ia_global;
                return pow(f * result, m);
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                float L = getLuminance(color);
                float sigmaIr = sigmaIa(color.r, exposure * Iav_r, L);
                float sigmaIg = sigmaIa(color.g, exposure * Iav_g, L);
                float sigmaIb = sigmaIa(color.b, exposure * Iav_b, L);
                color.r = color.r / (color.r + sigmaIr);
                color.g = color.g / (color.g + sigmaIg);
                color.b = color.b / (color.b + sigmaIb);
                color = clampedValue(color);
                out_color = gammaCorrect(color);
            }
            )glsl"
        );
    }

    virtual void setParameters(const Image *image) override {
        float Lmax = image->getMaximumLuminance();
        float Lav = image->getAverageLuminance();
        float Llav = image->getAverageLuminance();
        float Lmin = image->getMinimumLuminance();
        float k = (std::log(Lmax) - std::log(Llav)) / (std::log(Lmax) - std::log(Lmin));
        float m = 0.3f + 0.7f * std::pow(k, 1.4f);
        parameters.at("m").defaultValue = m;
        parameters.at("m").value = m;

        parameters["Iav_r"] = Parameter(image->getAverageIntensity().r(), "Iav_r");
        parameters["Iav_g"] = Parameter(image->getAverageIntensity().r(), "Iav_g");
        parameters["Iav_b"] = Parameter(image->getAverageIntensity().r(), "Iav_b");

        parameters["Lav"] = Parameter(Lav, "Lav");
    };

    void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
        const nanogui::Vector2i &size = image->getSize();
        *progress = 0.f;
        float delta = 1.f / (size.x() * size.y());

        float gamma = parameters.at("Gamma").value;
        float m = parameters.at("m").value;
        float f = parameters.at("f").value;
        float c = parameters.at("c").value;
        float a = parameters.at("a").value;
        float Iav_r = parameters.at("Iav_r").value;
        float Iav_g = parameters.at("Iav_g").value;
        float Iav_b = parameters.at("Iav_b").value;
        float Lav = parameters.at("Lav").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                Color3f col = map(color, exposure, m, f, c, a, Iav_r, Iav_g, Iav_b, Lav);
                col = col.clampedValue();
                col = col.gammaCorrect(gamma);
                dst[0] = (uint8_t) (255.f * col.r());
                dst[1] = (uint8_t) (255.f * col.g());
                dst[2] = (uint8_t) (255.f * col.b());
                dst += 3;
                *progress += delta;
            }
        }
    }

    float graph(float value) const override {
        float gamma = parameters.at("Gamma").value;
        float m = parameters.at("m").value;
        float f = parameters.at("f").value;
        float c = parameters.at("c").value;
        float a = parameters.at("a").value;
        float Iav_r = parameters.at("Iav_r").value;
        float Iav_g = parameters.at("Iav_g").value;
        float Iav_b = parameters.at("Iav_b").value;
        float Lav = parameters.at("Lav").value;

        value = map(Color3f(value), 1.f, m, f, c, a, Iav_r, Iav_g, Iav_b, Lav).getLuminance();
        value = clamp(value, 0.f, 1.f);
        value = std::pow(value, 1.f / gamma);
        return value;
    }

protected:
    Color3f map(const Color3f &col, float exposure, float m, float f, float c, float a, float Iav_r, float Iav_g, float Iav_b, float Lav) const {
        Color3f color = exposure * col;
        float L = color.getLuminance();
        float sigmaIr = sigmaIa(color.r(), exposure * Iav_r, L, exposure * Lav, c, a, m, f);
        float sigmaIg = sigmaIa(color.g(), exposure * Iav_g, L, exposure * Lav, c, a, m, f);
        float sigmaIb = sigmaIa(color.b(), exposure * Iav_b, L, exposure * Lav, c, a, m, f);

        color.r() = color.r() / (color.r() + sigmaIr);
        color.g() = color.g() / (color.g() + sigmaIg);
        color.b() = color.b() / (color.b() + sigmaIb);

        return color;
    }

    float sigmaIa(float Ia, float Iav_a, float L, float Lav, float c, float a, float m, float f) const {
        float Ia_local = c * Ia + (1.f - c) * L;
        float Ia_global = c * Iav_a + (1.f - c) * Lav;
        float result = a * Ia_local + (1.f - a) * Ia_global;
        return std::pow(f * result, m);
    }
};