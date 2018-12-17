/*
    src/filmic1.h -- Filmic 1 tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class Filmic1Operator : public TonemapOperator {
public:
    Filmic1Operator() : TonemapOperator() {
        name = "Filmic 1";
        description = "Filmic Mapping 1\n\nBy Jim Hejl and Richard Burgess-Dawson from the \"Filmic Tonemapping for Real-time Rendering\" Siggraph 2010 Course by Haarm-Pieter Duiker.";

        shader->init(
            "Filmic 1",

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
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                vec4 x = max(vec4(0.0), color - 0.004);
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

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                Color3f c = Color3f(map(color.r(), exposure),
                                    map(color.g(), exposure),
                                    map(color.b(), exposure));
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
        return map(value, 1.f);
    }

protected:
    float map(float v, float exposure) const {
        float value = exposure * v;
        value = std::max(0.f, value - 0.004f);
        return (value * (6.2f * value + 0.5f)) / (value * (6.2f * value + 1.7f) + 0.06f);
    }
};