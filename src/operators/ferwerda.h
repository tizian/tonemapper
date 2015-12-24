#pragma once

#include <tonemap.h>

class FerwerdaOperator : public TonemapOperator {
public:
	FerwerdaOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");
		parameters["max Ld"] = Parameter(200.f, 0.f, 300.f, "maxLd");
		parameters["k"] = Parameter(0.5f, 0.f, 1.f, "k");

		name = "Ferwerda";

		shader->init(
			"Ferwerda",

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
			"	 float Lda = maxLd / 2.0;\n"
			"	 float m = pow((1.219 + pow(Lda, 0.4)) / (1.219 + pow(Lwa * exposure, 0.4)), 2.5) / maxLd;\n"
			"	 color = m * color;\n"
			"    out_color = vec4(correct(color.r), correct(color.g), correct(color.b), 1);\n"
			"}"
		);
	}

	inline float tp(float La) const {
		float logLa = std::log10(La);
		float result;
		if (logLa <= -2.6f) {
			result =  -0.72f;
		}
		else if (logLa >= 1.9f) {
			result =  logLa - 1.255f;
		}
		else {
			result =  std::pow(0.249f * logLa + 0.65f, 2.7f) - 0.72f;
		}
		return std::pow(result, 10.f);
	}

	inline float ts(float La) const {
		float logLa = std::log10(La);
		float result;
		if (logLa <= -3.94f) {
			result =  -2.86f;
		}
		else if (logLa >= -1.44f) {
			result = logLa - 0.395f;
		}
		else {
			result = std::pow(0.405f * logLa + 1.6f, 2.18f) - 2.86f;
		}
		return std::pow(result, 10.f);
	}

	virtual float correct(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float Lwa = parameters.at("Lwa").value;
		float maxLd = parameters.at("max Ld").value;
		float Lda = maxLd / 2.f;
		value *= exposure;

		float Ldp = tp(Lda) / tp(exposure * Lwa) * value;
		float Lds = ts(Lda) / ts(exposure * Lwa) * value;

		float k = parameters.at("k").value;
		value = Ldp + k * Lds;

		value = value / maxLd;
		
		return std::pow(value, 1.f/gamma);
	}

	virtual void setParameters(const Image *image) {
		parameters["Lwa"] = Parameter(image->getLogAverageLuminance(), "Lwa");
	};
};