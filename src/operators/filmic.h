#pragma once

#include <tonemap.h>

class FilmicOperator : public TonemapOperator {
public:
	FilmicOperator() : TonemapOperator() {
		name = "Filmic";

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

protected:
	virtual float map(float value, float exposure) const override {
		value *= exposure;
		value = std::max(0.f, value - 0.004f);
		return (value * (6.2f * value + 0.5f)) / (value * (6.2f * value + 1.7f) + 0.06f);
	}
};