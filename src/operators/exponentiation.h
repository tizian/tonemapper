#pragma once

#include <tonemap.h>

class ExponentiationOperator : public TonemapOperator {
public:
	ExponentiationOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");
		parameters["p"] = Parameter(0.5f, 0.f, 1.f, "p");

		name = "Exponentiation";

		shader->init(
			"Exponentiation",

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
			"uniform float maxLum;\n"
			"uniform float p;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"float correct(float value) {\n"
			"    return pow(value, p/gamma);\n"
			"}\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = color / (exposure * maxLum);\n"
			"    out_color = vec4(correct(color.r), correct(color.g), correct(color.b), 1);\n"
			"}"
		);
	}

	virtual float correct(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float maxLum = parameters.at("maxLum").value;
		float p = parameters.at("p").value;
		value *= exposure;
		value = value / (exposure * maxLum);
		return std::pow(value, p/gamma);
	}

	virtual void setParameters(const Image *image) {
		parameters["maxLum"] = Parameter(image->getMaximumLuminance(), "maxLum");
	};
};