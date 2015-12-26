#pragma once

#include <tonemap.h>

class ExponentiationOperator : public TonemapOperator {
public:
	ExponentiationOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["p"] = Parameter(0.5f, 0.f, 1.f, "p", "Curve exponent parameter");

		name = "Exponentiation";
		description = "Exponentiation Mapping\n\nDiscussed in \"Quantization Techniques for Visualization of High Dynamic Range Pictures\" by Schlick 1994.";

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
			"uniform float Lmax;\n"
			"uniform float p;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"\n"
			"vec4 clampedValue(vec4 color) {\n"
			"	 color.a = 1.0;\n"
			"	 return clamp(color, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"vec4 gammaCorrectPlus(vec4 color) {\n"
			"	 return pow(color, vec4(p/gamma));\n"
			"}\n"
			"\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = color / (exposure * Lmax);\n"
			"	 color = gammaCorrectPlus(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		parameters["Lmax"] = Parameter(image->getMaximumLuminance(), "Lmax");
	};

protected:
	virtual float map(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float Lmax = parameters.at("Lmax").value;
		float p = parameters.at("p").value;

		value *= exposure;
		value = value / (exposure * Lmax);
		return gammaCorrect(value, gamma/p);
	}
};