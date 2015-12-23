#pragma once

#include <tonemap.h>

class TumblinRushmeierOperator : public TonemapOperator {
public:
	TumblinRushmeierOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");

		parameters["maxLd"] = Parameter(86.f, 1.f, 200.f, "maxLd");
		parameters["maxC"] = Parameter(35.f, 1.f, 100.f, "maxC");

		name = "Tumblin-Rushmeier";

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
			"uniform float avgL;\n"
			"uniform float maxLd;\n"
			"uniform float maxC;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"float correct(float value) {\n"
			"    return pow(value, 1/gamma);\n"
			"}\n"
			"void main() {\n"
			"	 float log10Lrw = log(exposure * avgL)/log(10.0);\n"
			"	 float alpha_rw = 0.4 * log10Lrw + 2.92;\n"
			"	 float beta_rw = -0.4 * log10Lrw*log10Lrw - 2.584 * log10Lrw + 2.0208;\n"
			"	 float log10Ld = log(maxLd / sqrt(maxC))/log(10.0);\n"
			"	 float alpha_d = 0.4 * log10Ld + 2.92;\n"
			"	 float beta_d = -0.4 * log10Ld*log10Ld - 2.584 * log10Ld + 2.0208;\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = pow(color, vec4(alpha_rw/alpha_d)) / maxLd * pow(10, (beta_rw - beta_d) / alpha_d) - (1 / maxC);\n"
			"    out_color = vec4(correct(color.r), correct(color.g), correct(color.b), 1);\n"
			"}"
		);
	}

	virtual float correct(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		value *= exposure;

		float avgL = parameters.at("avgL").value;
		float log10Lrw = std::log10(exposure * avgL);
		float alpha_rw = 0.4f * log10Lrw + 2.92f;
		float beta_rw = -0.4f * log10Lrw*log10Lrw - 2.584f * log10Lrw + 2.0208f;

		float maxLd = parameters.at("maxLd").value;
		float maxC = parameters.at("maxC").value;
		float log10Ld = std::log10(maxLd / std::sqrt(maxC));
		float alpha_d = 0.4f * log10Ld + 2.92f;
		float beta_d = -0.4f * log10Ld*log10Ld - 2.584f * log10Ld + 2.0208f;

		value = std::pow(value, alpha_rw/alpha_d) / maxLd * std::pow(10, (beta_rw - beta_d) / alpha_d) - (1.f / maxC);
		
		return std::pow(value, 1.f/gamma);
	}

	virtual void setParameters(const Image *image) {
		parameters["avgL"] = Parameter(image->getAverageLuminance(), "avgL");
	};
};