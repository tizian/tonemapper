/*
    src/insomniac.h -- Insomniac tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class InsomniacOperator : public TonemapOperator {
public:
    InsomniacOperator() : TonemapOperator() {
        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
        parameters["w"] = Parameter(10.f, 0.f, 20.f, "w", "White point\nMinimal value that is mapped to 1.");
        parameters["b"] = Parameter(0.1f, 0.f, 2.f, "b", "Black point\nMaximal value that is mapped to 0.");
        parameters["t"] = Parameter(0.7f, 0.f, 1.f, "t", "Toe strength\nAmount of blending between a straight-line curve and a purely asymptotic curve for the toe.");
        parameters["s"] = Parameter(0.8f, 0.f, 1.f, "s", "Shoulder strength\nAmount of blending between a straight-line curve and a purely asymptotic curve for the shoulder.");
        parameters["c"] = Parameter(2.f, 0.f, 10.f, "c", "Cross-over point\nPoint where the toe and shoulder are pieced together into a single curve.");

        name = "Insomniac (Day)";
        description = "Insomniac Mapping\n\nFrom \"An efficient and user-friendly tone mapping operator\" by Mike Day (Insomniac Games).";

        shader->init(
            "Insomniac",

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
            uniform float w;
            uniform float b;
            uniform float t;
            uniform float s;
            uniform float c;
            uniform float Lavg;
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            vec4 gammaCorrect(vec4 color) {
                return pow(color, vec4(1.0/gamma));
            }

            float tonemap(float x, float k) {
                if (x < c) {
                    return k * (1.0-t)*(x-b) / (c - (1.0-t)*b - t*x);
                }
                else {
                    return (1.0-k)*(x-c) / (s*x + (1.0-s)*w - c) + k;
                }
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                color = color / Lavg;
                float k = (1.0-t)*(c-b) / ((1.0-s)*(w-c) + (1.0-t)*(c-b));
                color = vec4(tonemap(color.r, k), tonemap(color.g, k), tonemap(color.b, k), 1.0);
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
        float w = parameters.at("w").value;
        float b = parameters.at("b").value;
        float t = parameters.at("t").value;
        float s = parameters.at("s").value;
        float c = parameters.at("c").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                Color3f col = Color3f(  map(color.r(), exposure, gamma, Lavg, w, b, t, s, c),
                                        map(color.g(), exposure, gamma, Lavg, w, b, t, s, c),
                                        map(color.b(), exposure, gamma, Lavg, w, b, t, s, c));
                col = col.clampedValue();
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
        float Lavg = parameters.at("Lavg").value;
        float w = parameters.at("w").value;
        float b = parameters.at("b").value;
        float t = parameters.at("t").value;
        float s = parameters.at("s").value;
        float c = parameters.at("c").value;

        return map(value, 1.f, gamma, Lavg, w, b, t, s, c);
    }

protected:
    float map(float v, float exposure, float gamma, float Lavg, float w, float b, float t, float s, float c) const {
        float value = exposure * v;
        value = value / (exposure * Lavg);

        float k = (1.f-t)*(c-b) / ((1.f-s)*(w-c) + (1.f-t)*(c-b));

        if (value < c) {
            value = k * (1.f-t)*(value-b) / (c - (1.f-t)*b - t*value);
        }
        else {
            value = (1.f-k)*(value-c) / (s*value + (1.f-s)*w - c) + k;
        }

        return std::pow(value, 1.f / gamma);
    }
};