#pragma once

#include <tonemap.h>

class ExtendedReinhardOperator : public TonemapOperator {
public:
	ExtendedReinhardOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");
		parameters["L_white"] = Parameter(1.f, 0.f, 5.f, "L_white");

		name = "Reinhard (Extended)";

		shader->init(
			"ExtendedReinhard",

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
			"uniform float gamma;\n"
			"uniform float L_white;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"float correct(float value) {\n"
			"    return pow(value, 1/gamma);\n"
			"}\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = (color * (1 + color / (L_white * L_white))) / (1 + color);\n"
			"    out_color = vec4(correct(color.r), correct(color.g), correct(color.b), 1);\n"
			"}"
		);
	}

	virtual float correct(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float L_white = parameters.at("L_white").value;
		value *= exposure;
		value = (value * (1.f + value / (L_white * L_white))) / (1.f + value);
		return std::pow(value, 1.f/gamma);
	}
};