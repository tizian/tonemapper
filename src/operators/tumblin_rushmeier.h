#pragma once

#include <tonemap.h>

class TumblinRushmeierOperator : public TonemapOperator {
public:
	TumblinRushmeierOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");

		parameters["Ldmax"] = Parameter(86.f, 1.f, 200.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");
		parameters["Cmax"] = Parameter(35.f, 1.f, 100.f, "Cmax", "Maximum contrast ratio between on-screen luminances");

		name = "Tumblin-Rushmeier";
		description = "Tumblin-Rushmeier Mapping\n\nProposed in\"Tone Reproduction for Realistic Images\" by Tumblin and Rushmeier 1993.";

		shader->init(
			"TumblinRushmeier",

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
			"uniform float Lavg;\n"
			"uniform float Ldmax;\n"
			"uniform float Cmax;\n"
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
			"	 float log10Lrw = log(exposure * Lavg)/log(10.0);\n"
			"	 float alpha_rw = 0.4 * log10Lrw + 2.92;\n"
			"	 float beta_rw = -0.4 * log10Lrw*log10Lrw - 2.584 * log10Lrw + 2.0208;\n"
			"	 float log10Ld = log(Ldmax / sqrt(Cmax))/log(10.0);\n"
			"	 float alpha_d = 0.4 * log10Ld + 2.92;\n"
			"	 float beta_d = -0.4 * log10Ld*log10Ld - 2.584 * log10Ld + 2.0208;\n"
			"\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = pow(color, vec4(alpha_rw/alpha_d)) / Ldmax * pow(10.0, (beta_rw - beta_d) / alpha_d) - (1 / Cmax);\n"
			"	 color = gammaCorrect(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		parameters["Lavg"] = Parameter(image->getAverageLuminance(), "Lavg");
	};

protected:
	virtual float map(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;

		float Lavg = parameters.at("Lavg").value;
		float log10Lrw = std::log10(exposure * Lavg);
		float alpha_rw = 0.4f * log10Lrw + 2.92f;
		float beta_rw = -0.4f * log10Lrw*log10Lrw - 2.584f * log10Lrw + 2.0208f;

		float Ldmax = parameters.at("Ldmax").value;
		float Cmax = parameters.at("Cmax").value;
		float log10Ld = std::log10(Ldmax / std::sqrt(Cmax));
		float alpha_d = 0.4f * log10Ld + 2.92f;
		float beta_d = -0.4f * log10Ld*log10Ld - 2.584f * log10Ld + 2.0208f;


		value *= exposure;
		value = std::pow(value, alpha_rw/alpha_d) / Ldmax * std::pow(10.f, (beta_rw - beta_d) / alpha_d) - (1.f / Cmax);
		return gammaCorrect(value, gamma);
	}
};