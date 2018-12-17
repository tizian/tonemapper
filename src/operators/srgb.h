/*
    src/srgb.h -- sRGB tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class SRGBOperator : public TonemapOperator {
public:
    SRGBOperator() : TonemapOperator() {
        name = "sRGB";
        description = "sRGB\n\nConversion to the sRGB color space.";

        shader->init(
            "sRGB",

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

            float toSRGB(float value) {
                if (value < 0.0031308)
                    return 12.92 * value;
                return 1.055 * pow(value, 0.41666) - 0.055;
            }

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                color = vec4(toSRGB(color.r), toSRGB(color.g), toSRGB(color.b), 1.0);
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
        value = map(value, 1.f);
        value = clamp(value, 0.f, 1.f);
        return value;
    }

protected:
    float map(float v, float exposure) const {
        float value = exposure * v;
        if (value < 0.0031308f) {
            return 12.92f * value;
        }
        return 1.055f * std::pow(value, 0.41666f) - 0.055f;
    }
};