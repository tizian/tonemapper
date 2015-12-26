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

protected:
	virtual float map(float value, float exposure) const override {
		value *= exposure;
		if (value < 0.0031308f) {
			return 12.92f * value;
		}
		return 1.055f * std::pow(value, 0.41666f) - 0.055f;
	}
};