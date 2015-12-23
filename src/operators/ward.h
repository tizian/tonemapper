#pragma once

#include <tonemap.h>

class WardOperator : public TonemapOperator {
public:
	WardOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");
		parameters["max Ld"] = Parameter(100.f, 0.f, 200.f, "maxLd");

		name = "Ward";

		shader->init(
			"Ward",

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
			"uniform float Lwa;\n"
			"uniform float maxLd;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"float correct(float value) {\n"
			"    return pow(value, 1/gamma);\n"
			"}\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 float m = pow((1.219 + pow(maxLd/2.0, 0.4)) / (1.219 + pow(Lwa * exposure, 0.4)), 2.5) / maxLd;\n"
			"	 color = m * color;\n"
			"    out_color = vec4(correct(color.r), correct(color.g), correct(color.b), 1);\n"
			"}"
		);
	}

	virtual float correct(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float Lwa = parameters.at("Lwa").value;
		float maxLd = parameters.at("max Ld").value;
		value *= exposure;
		float m = std::pow((1.219f + std::pow(maxLd/2.f, 0.4f)) / (1.219f + std::pow(Lwa * exposure, 0.4f)), 2.5f) / maxLd;
		value = m * value;
		return std::pow(value, 1.f/gamma);
	}

	virtual void setParameters(const Image *image) {
		parameters["Lwa"] = Parameter(image->getLogAverageLuminance(), "Lwa");
	};
};