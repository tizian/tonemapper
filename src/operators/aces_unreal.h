/*
    src/filmic1.h -- ACES tonemapping operator, Unreal engine version

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class AcesUnrealOperator : public TonemapOperator {
public:
    AcesUnrealOperator() : TonemapOperator() {
        name = "ACES (Unreal)";
        description = "Unreal 3 color grading curve adapted by Romain Guy to be close to ACES curve";

        shader->init(
            "ACES (Unreal)",

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

            // Taken from: https://www.shadertoy.com/view/llXyWr by Romain Guy

            uniform sampler2D source;
            uniform float exposure;
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            void main() {
                vec4 x = exposure * texture(source, uv);
                x = x / (x + 0.155) * 1.019;
                out_color = clampedValue(x);
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
        float x = exposure * v;
        return x / (x + 0.155f) * 1.019f;
    }
};