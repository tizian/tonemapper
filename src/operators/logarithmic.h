#pragma once

#include <tonemap.h>

class LogarithmicOperator : public TonemapOperator {
public:
	LogarithmicOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");
		parameters["p"] = Parameter(1.f, 0.f, 20.f, "p");
		parameters["q"] = Parameter(1.f, 0.f, 20.f, "q");

		name = "Logarithmic";

		shader->init(
			"Logarithmic",

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
			"uniform float q;\n"
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
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = (log(1.0 + p * color)/log(10.0)) / (log(1.0 + q * exposure * maxLum)/log(10.0));\n"
			"	 color = gammaCorrect(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) {
		parameters["maxLum"] = Parameter(image->getMaximumLuminance(), "maxLum");
	};

protected:
	virtual float map(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float maxLum = parameters.at("maxLum").value;
		float p = parameters.at("p").value;
		float q = parameters.at("q").value;

		value *= exposure;
		value = std::log10(1.f + p * value) / std::log10(1.f + q * exposure * maxLum);
		return gammaCorrect(value, gamma);
	}
};