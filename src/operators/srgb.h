#pragma once

#include <operator.h>

class SRGBOperator : public ToneMappingOperator {
public:
	SRGBOperator() : ToneMappingOperator() {
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
			"float correct(float value) {\n"
			"	 if (value < 0.0031308)\n"
			"	 	 return 12.92 * value;\n"
			"    return 1.055 * pow(value, 0.41666) - 0.055;\n"
			"}\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"    out_color = vec4(correct(color.r), correct(color.g), correct(color.b), 1);\n"
			"}"
		);
	}

	virtual float correct(float value, float exposure) const {
		value *= exposure;
		if (value < 0.0031308) {
			return 12.92 * value;
		}
		return 1.055 * std::pow(value, 0.41666) - 0.055;
	}

	std::string getString() const {
		return "sRGB";
	}
};