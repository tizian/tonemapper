/*
    src/filmic2.h -- Filmic 2 tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class Filmic2Operator : public TonemapOperator {
public:
    Filmic2Operator() : TonemapOperator() {
        parameters["Cutoff"] = Parameter(0.025, 0.f, 0.5f, "cutoff", "Transition into compressed blacks");

        name = "Filmic 2";
        description = "Filmic Mapping 2\n\nBy Graham Aldridge from \"Approximating Film with Tonemapping\".";

        shader->init(
            "Filmic 2",

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
            uniform float cutoff;
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                vec4 x = color + (cutoff * 2.0 - color) * clamp(cutoff * 2.0 - color, 0.0, 1.0) * (0.25 / cutoff) - cutoff;
                color = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
                out_color = clampedValue(color);
            }
            )glsl"
        );
    }

    void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
        const nanogui::Vector2i &size = image->getSize();
        *progress = 0.f;
        float delta = 1.f / (size.x() * size.y());

        float cutoff = parameters.at("Cutoff").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                Color3f c = Color3f(map(color.r(), cutoff, exposure),
                                    map(color.g(), cutoff, exposure),
                                    map(color.b(), cutoff, exposure));
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
        float cutoff = parameters.at("Cutoff").value;
        return map(value, cutoff, 1.f);
    }

protected:
    float map(float v, float cutoff, float exposure) const {
        float value = exposure * v;
        value += (cutoff * 2.0 - value) * clamp(cutoff * 2.0 - value, 0.0, 1.0) * (0.25 / cutoff) - cutoff;
        return (value * (6.2f * value + 0.5f)) / (value * (6.2f * value + 1.7f) + 0.06f);
    }
};