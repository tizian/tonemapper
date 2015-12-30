/*
    src/filmic.h -- Filmic tonemapping operator
    
    Copyright (c) 2015 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license. 
*/

#pragma once

#include <tonemap.h>

class FilmicOperator : public TonemapOperator {
public:
	FilmicOperator() : TonemapOperator() {
		name = "Filmic";
		description = "Filmic Mapping\n\nBy Jim Hejl and Richard Burgess-Dawson from the \"Filmic Tonemapping for Real-time Rendering\" Siggraph 2010 Course by Haarm-Pieter Duiker.";

		shader->init(
			"Filmic",

			"#version 330\n"
			"in vec2 position;\n"
			"out vec2 uv;\n"
			"void main() {\n"
			"    gl_Position = vec4(position.x*2-1, position.y*2-1, 0.0, 1.0);\n"
			"    uv = vec2(position.x, 1-position.y);\n"
			"}",

			"#version 330\n"
			"uniform sampler2D source;\n"
			"uniform float exposure;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"\n"
			"vec4 clampedValue(vec4 color) {\n"
			"	 color.a = 1.0;\n"
			"	 return clamp(color, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 vec4 x = max(vec4(0.0), color - 0.004);\n"
			"	 color = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				float colorR = map(color.r(), exposure);
				float colorG = map(color.g(), exposure);
				float colorB = map(color.b(), exposure);
				dst[0] = (uint8_t) clamp(255.f * colorR, 0.f, 255.f);
				dst[1] = (uint8_t) clamp(255.f * colorG, 0.f, 255.f);
				dst[2] = (uint8_t) clamp(255.f * colorB, 0.f, 255.f);
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