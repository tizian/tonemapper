/*
    src/gran_turismo.h -- Gran Turismo 2017 tonemapping operator

    Copyright(c) 2017 by Hajime UCHIMURA @ Polyphony Digital Inc.
    https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
    https://www.desmos.com/calculator/gslcdxvipg - English Translation by Romain Guy

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class GranTurismoOperator : public TonemapOperator {
public:
    GranTurismoOperator() : TonemapOperator() {
        parameters["P"] = Parameter(1.f, 1.f, 100.f, "P_in", "Maximum Brightness");
        parameters["a"] = Parameter(1.f, 0.f, 5.f, "a_in", "Contrast");
        parameters["m"] = Parameter(0.22f, 0.f, 1.f, "m_in", "Linear section start");
        parameters["l"] = Parameter(0.4f, 0.01f, 0.99f, "l_in", "Linear section length");
        parameters["c"] = Parameter(1.33f, 1.f, 3.f, "c_in", "Black tightness-c");
        parameters["b"] = Parameter(0.f, 0.f, 1.f, "b_in", "Black tightness-b");

        name = "Gran Turismo";
        description = "Gran Turismo - by Hajime Uchimura\n";

        shader->init(
            "Gran Turismo",

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

            // GT Tonemap
            // Copyright(c) 2017 by Hajime Uchimura @ Polyphony Digital Inc.
            // Translated to GLSL by Nick Porcino from
            // Romain Guy's translation of https://www.desmos.com/calculator/gslcdxvipg

            in vec2 uv;
            out vec4 out_color;

            uniform float exposure;

            // Maximum brightness, 1 .. 100
            uniform float P_in;

            // Contrast 1 .. 5
            uniform float a_in;

            // Linear section start 0 .. 1
            uniform float m_in;

            // Linear section length 0 .. 1
            uniform float l_in;

            // Black tightness 1 .. 3, 0 .. 1
            uniform float c_in;
            uniform float b_in;

            uniform sampler2D source;

            vec4 clampedValue(vec4 color) {
                 color.a = 1.0;
                 return clamp(color, 0.0, 1.0);
            }

            void main() {
                vec4 P = vec4(P_in);
                vec4 a = vec4(a_in);
                vec4 m = vec4(m_in);
                vec4 l = vec4(l_in);
                vec4 c = vec4(c_in);
                vec4 b = vec4(b_in);

                // Normalized pixel coordinates (from 0 to 1)
                vec4 x = exposure * texture(source, uv);

                // Linear Region Computation
                // l0 is the linear length after scale
                vec4 l0 = ((P - m) * l) / a;
                vec4 L0 = m - (m / a);
                vec4 L1 = m + (1 - m) / a;
                vec4 Lx = m + a * (x - m);

                // Toe
                vec4 Tx = m * pow(x / m, c) + b;

                // Shoulder
                vec4 S0 = m + l0;
                vec4 S1 = m + a * l0;
                vec4 C2 = (a * P) / (P - S1);
                vec4 Sx = P - (P - S1) * exp(-(C2 * (x - S0) / P));

                // Toe weight
                vec4 w0 = vec4(1.0) - smoothstep(vec4(0), m, x);
                // Shoulder weight
                vec4 w2 = smoothstep(m + l0, m + l0, x);
                // Linear weight
                vec4 w1 = vec4(1) - w0 - w2;

                out_color = clampedValue(pow(Tx * w0 + Lx * w1 + Sx * w2, vec4(1.0/2.2)));
            }
            )glsl"
        );
    }

    void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
        const nanogui::Vector2i &size = image->getSize();
        *progress = 0.f;
        float delta = 1.f / (size.x() * size.y());

        float P = parameters.at("P").value;
        float a = parameters.at("a").value;
        float m = parameters.at("m").value;
        float l = parameters.at("l").value;
        float c = parameters.at("c").value;
        float b = parameters.at("b").value;

        for (int i = 0; i < size.y(); ++i) {
            for (int j = 0; j < size.x(); ++j) {
                const Color3f &color = image->ref(i, j);
                Color3f c3 = Color3f(map(color.r(), P, a, m, l, c, b, exposure),
                                    map(color.g(), P, a, m, l, c, b, exposure),
                                    map(color.b(), P, a, m, l, c, b, exposure));
                c3 = c3.clampedValue();
                dst[0] = (uint8_t) (255.f * c3.r());
                dst[1] = (uint8_t) (255.f * c3.g());
                dst[2] = (uint8_t) (255.f * c3.b());
                dst += 3;
                *progress += delta;
            }
        }
    }

    float graph(float value) const override {
        float P = parameters.at("P").value;
        float a = parameters.at("a").value;
        float m = parameters.at("m").value;
        float l = parameters.at("l").value;
        float c = parameters.at("c").value;
        float b = parameters.at("b").value;
        return map(value, P, a, m, l, c, b, 1.f);
    }

protected:
    float smoothstep(float edge0, float edge1, float x) const {
        // Scale, bias and saturate x to 0..1 range
        x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        // Evaluate polynomial
        return x*x*(3 - 2 * x);
    }

    float map(float v, float P, float a, float m, float l, float c, float b, float exposure) const {
        float x = exposure * v;

        // Linear Region Computation
        // l0 is the linear length after scale
        float l0 = ((P - m) * l) / a;
        // (These two are presented in the source, but unused during actual computation.)
        // float L0 = m - (m / a);
        // float L1 = m + (1 - m) / a;
        float Lx = m + a * (x - m);

        // Toe
        float Tx = m * pow(x / m, c) + b;

        // Shoulder
        float S0 = m + l0;
        float S1 = m + a * l0;
        float C2 = (a * P) / (P - S1);
        float Sx = P - (P - S1) * exp(-(C2 * (x - S0) / P));

        // Toe weight
        float w0 = 1.f - smoothstep(0.f, m, x);
        // Shoulder weight
        float w2 = smoothstep(m + l0, m + l0, x);
        // Linear weight
        float w1 = 1.f - w0 - w2;

        float out_val = pow(Tx * w0 + Lx * w1 + Sx * w2, 1.f/2.2f);
        return out_val < 0.f ? 0 : (out_val > 1.f ? 1.f : out_val);
    }
};
