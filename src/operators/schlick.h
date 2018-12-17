/*
    src/schlick.h -- Schlick tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class SchlickOperator : public TonemapOperator {
public:
    SchlickOperator() : TonemapOperator() {
        parameters["p"] = Parameter(200.f, 1.f, 1000.f, "p", "Rational mapping curve parameter");

        name = "Schlick";
        description = "Schlick Mapping\n\nProposed in \"Quantization Techniques for Visualization of High Dynamic Range Pictures\" by Schlick 1994.";

        shader->init(
            "Schlick",

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
            uniform float Lmax;
            uniform float p;
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                color = p * color / (p * color - color + exposure * Lmax);
                out_color = clampedValue(color);
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

        float Lmax = parameters.at("Lmax").value;
        float p = parameters.at("p").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                Color3f c = Color3f(map(color.r(), exposure, Lmax, p),
                                    map(color.g(), exposure, Lmax, p),
                                    map(color.b(), exposure, Lmax, p));
                c = c.clampedValue();
                dst[0] = (uint8_t) (255.f * c.r());
                dst[1] = (uint8_t) (255.f * c.g());
                dst[2] = (uint8_t) (255.f * c.b());
                dst += 3;
                *progress += delta;
            }
        }
    }

    float graph(float value) const override {
        float Lmax = parameters.at("Lmax").value;
        float p = parameters.at("p").value;

        value = map(value, 1.f, Lmax, p);
        value = clamp(value, 0.f, 1.f);
        return value;
    }

protected:
    float map(float v, float exposure, float Lmax, float p) const {
        float value = exposure * v;
        value = p * value / (p * value - value + exposure * Lmax);
        return value;
    }
};