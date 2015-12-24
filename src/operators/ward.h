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
			"\n"
			"vec4 clampedValue(vec4 color) {\n"
			"	 color.a = 1.0;\n"
			"	 return clamp(color, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"vec4 gammaCorrect(vec4 color) {\n"
			"	 return pow(color, vec4(1.0/gamma));\n"
			"}\n"
			"\n"
			"void main() {\n"
			"	 float Lda = maxLd / 2.0;\n"
			"	 float m = pow((1.219 + pow(Lda, 0.4)) / (1.219 + pow(Lwa * exposure, 0.4)), 2.5) / maxLd;\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = m * color;\n"
			"	 color = gammaCorrect(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) {
		parameters["Lwa"] = Parameter(image->getLogAverageLuminance(), "Lwa");
	};

protected:
	virtual float map(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float Lwa = parameters.at("Lwa").value;
		float maxLd = parameters.at("max Ld").value;
		float Lda = maxLd / 2.f;
		float m = std::pow((1.219f + std::pow(Lda, 0.4f)) / (1.219f + std::pow(Lwa * exposure, 0.4f)), 2.5f) / maxLd;

		value *= exposure;
		value = m * value;
		return gammaCorrect(value, gamma);
	}
};