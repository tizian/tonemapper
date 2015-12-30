/*
    src/srgb.h -- sRGB tonemapping operator
    
    Copyright (c) 2015 Tizian Zeltner

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
			"float toSRGB(float value) {\n"
			"	 if (value < 0.0031308)\n"
			"	 	 return 12.92 * value;\n"
			"    return 1.055 * pow(value, 0.41666) - 0.055;\n"
			"}\n"
			"\n"
			"vec4 clampedValue(vec4 color) {\n"
			"	 color.a = 1.0;\n"
			"	 return clamp(color, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"    color = vec4(toSRGB(color.r), toSRGB(color.g), toSRGB(color.b), 1.0);\n"
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
		if (value < 0.0031308f) {
			return 12.92f * value;
		}
		return 1.055f * std::pow(value, 0.41666f) - 0.055f;
	}
};