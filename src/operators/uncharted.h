/*
    src/uncharted.h -- Uncharted tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class UnchartedOperator : public TonemapOperator {
public:
    UnchartedOperator() : TonemapOperator() {
        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
        parameters["A"] = Parameter(0.22f, 0.f, 1.f, "A", "Shoulder strength curve parameter");
        parameters["B"] = Parameter(0.3f, 0.f, 1.f, "B", "Linear strength curve parameter");
        parameters["C"] = Parameter(0.1f, 0.f, 1.f, "C", "Linear angle curve parameter");
        parameters["D"] = Parameter(0.2f, 0.f, 1.f, "D", "Toe strength curve parameter");
        parameters["E"] = Parameter(0.01f, 0.f, 1.f, "E", "Toe numerator curve parameter");
        parameters["F"] = Parameter(0.3f, 0.f, 1.f, "F", "Toe denominator curve parameter");
        parameters["W"] = Parameter(11.2f, 0.f, 20.f, "W", "White point\nMinimal value that is mapped to 1.");

        name = "Uncharted (Hable)";
        description = "Uncharted Mapping\n\nBy John Hable from the \"Filmic Tonemapping for Real-time Rendering\" Siggraph 2010 Course by Haarm-Pieter Duiker.";

        shader->init(
            "Uncharted",

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
            uniform float A;
            uniform float B;
            uniform float C;
            uniform float D;
            uniform float E;
            uniform float F;
            uniform float W;
            in vec2 uv;
            out vec4 out_color;

            vec4 clampedValue(vec4 color) {
                color.a = 1.0;
                return clamp(color, 0.0, 1.0);
            }

            vec4 gammaCorrect(vec4 color) {
                return pow(color, vec4(1.0/gamma));
            }

            vec4 tonemap(vec4 x) {
                return ((x * (A*x + C*B) + D*E) / (x * (A*x+B) + D*F)) - E/F;
            }

            void main() {
                vec4 color = exposure * texture(source, uv);
                float exposureBias = 2.0;
                vec4 curr = tonemap(exposureBias * color);
                vec4 whiteScale = 1.0 / tonemap(vec4(W));
                color = curr * whiteScale;
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
        float A = parameters.at("A").value;
        float B = parameters.at("B").value;
        float C = parameters.at("C").value;
        float D = parameters.at("D").value;
        float E = parameters.at("E").value;
        float F = parameters.at("F").value;
        float W = parameters.at("W").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                Color3f c = Color3f(map(color.r(), exposure, A, B, C, D, E, F, W),
                                    map(color.g(), exposure, A, B, C, D, E, F, W),
                                    map(color.b(), exposure, A, B, C, D, E, F, W));
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
        float A = parameters.at("A").value;
        float B = parameters.at("B").value;
        float C = parameters.at("C").value;
        float D = parameters.at("D").value;
        float E = parameters.at("E").value;
        float F = parameters.at("F").value;
        float W = parameters.at("W").value;

        value = map(value, 1.f, A, B, C, D, E, F, W);
        value = clamp(value, 0.f, 1.f);
        value = std::pow(value, 1.f / gamma);
        return value;
    }

protected:
    float map(float v, float exposure, float A, float B, float C, float D, float E, float F, float W) const {
        float value = exposure * v;
        float exposureBias = 2.f;
        value = mapAux(exposureBias * value, A, B, C, D, E, F);
        float whiteScale = 1.f / mapAux(W, A, B, C, D, E, F);
        value = value * whiteScale;
        return value;
    }

protected:
    float mapAux(float x, float A, float B, float C, float D, float E, float F) const {
        return ((x * (A*x + C*B) + D*E) / (x * (A*x+B) + D*F)) - E/F;
    }
};