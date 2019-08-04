/*
    src/exponential.h -- AMD tonemapping operator

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class AmdOperator : public TonemapOperator {
public:
    AmdOperator() : TonemapOperator() {
        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
        parameters["a"] = Parameter(1.6f, 1.f, 2.f, "a", "Contrast");
        parameters["d"] = Parameter(0.977f, 0.01f, 2.f, "d", "Shoulder");
        parameters["hdrMax"] = Parameter(8.f, 1.f, 10.f, "hdrMax", "hdrMax");
        parameters["midIn"] = Parameter(0.18f, 0.f, 1.f, "midIn", "midIn");
        parameters["midOut"] = Parameter(0.267f, 0.f, 1.f, "midOut", "midOut");

        name = "AMD (Lottes)";
        description = "AMD (Lottes)\n\nAMD operator from \"Advanced Techniques and Optimization of HDR Color Pipelines\",2016 by Timothy Lottes";

        shader->init(
            "AMD (Lottes)",

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
            uniform float gamma;
            uniform float a;
            uniform float d;
            uniform float hdrMax;
            uniform float midIn;
            uniform float midOut;
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            vec4 gammaCorrect(vec4 color) {
                return pow(color, vec4(1.0/gamma));
            }

            float map(float x) {
                float b = (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
                          ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
                float c = (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
                          ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

                return pow(x, a) / (pow(x, a * d) * b + c);
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                color = vec4(map(color.x), map(color.y), map(color.z), color.w);
                color = clampedValue(color);
                out_color = gammaCorrect(color);
            }
            )glsl"
        );
    }

    void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
        const nanogui::Vector2i &size = image->getSize();
        *progress = 0.f;
        float delta = 1.f / (size.x() * size.y());

        float gamma = parameters.at("Gamma").value;
        float a = parameters.at("a").value;
        float d = parameters.at("d").value;
        float hdrMax = parameters.at("hdrMax").value;
        float midIn = parameters.at("midIn").value;
        float midOut = parameters.at("midOut").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                Color3f c = Color3f(map(color.r(), exposure, a, d, hdrMax, midIn, midOut),
                                    map(color.g(), exposure, a, d, hdrMax, midIn, midOut),
                                    map(color.b(), exposure, a, d, hdrMax, midIn, midOut));
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
        float a = parameters.at("a").value;
        float d = parameters.at("d").value;
        float hdrMax = parameters.at("hdrMax").value;
        float midIn = parameters.at("midIn").value;
        float midOut = parameters.at("midOut").value;

        value = map(value, 1.f, a, d, hdrMax, midIn, midOut);
        value = clamp(value, 0.f, 1.f);
        value = std::pow(value, 1.f / gamma);
        return value;
    }

protected:
    float map(float v, float exposure, float a, float d, float hdrMax, float midIn, float midOut) const {
        float x = exposure * v;

        float b = (-std::pow(midIn, a) + std::pow(hdrMax, a) * midOut) /
                  ((std::pow(hdrMax, a * d) - std::pow(midIn, a * d)) * midOut);
        float c = (std::pow(hdrMax, a * d) * std::pow(midIn, a) - std::pow(hdrMax, a) * std::pow(midIn, a * d) * midOut) /
                  ((std::pow(hdrMax, a * d) - std::pow(midIn, a * d)) * midOut);

        return std::pow(x, a) / (std::pow(x, a * d) * b + c);
    }
};