#pragma once

#include <tonemap.h>

class DragoOperator : public TonemapOperator {
public:
	DragoOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");
		parameters["slope"] = Parameter(4.5f, 0.f, 10.f, "slope");
		parameters["start"] = Parameter(0.018f, 0.f, 2.f, "start");
		parameters["Ldmax"] = Parameter(100.f, 0.f, 200.f, "Ldmax");
		parameters["b"] = Parameter(0.85f, 0.f, 1.f, "b");

		name = "Drago";

		shader->init(
			"Drago",

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
			"uniform float Ldmax;\n"
			"uniform float Lwa;\n"
			"uniform float Lwmax;\n"
			"uniform float b;\n"
			"uniform float slope;\n"
			"uniform float start;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"\n"
			"vec4 clampedValue(vec4 color) {\n"
			"	 color.a = 1.0;\n"
			"	 return clamp(color, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"float gammaCorrect(float v) {\n"
			"	 if (v <= start) {\n"
			"	 	 return slope * v;\n"
			"	 }\n"
			"	 else {\n"
			"	 	 return pow(1.099 * v, 0.9/gamma) - 0.099;\n"
			"	 }\n"
			"}\n"
			"\n"
			"void main() {\n"
			"	 float LwaP = exposure * Lwa / pow(1.0 + b - 0.85, 5);\n"
			"	 float LwmaxP = exposure * Lwmax / LwaP;\n"
			"    vec4 color = exposure * texture(source, uv) / LwaP;\n"
			"	 float exponent = log(b) / log(0.5);\n"
			"	 float c1 = (0.01 * Ldmax) / (log(1 + LwmaxP)/log(10.0));\n"
			"	 vec4 c2 = log(color + 1) / log(2.0 + 8 * (pow(color / LwmaxP, vec4(exponent))));\n"
			"	 color = c1 * c2;\n"
			"	 color = vec4(gammaCorrect(color.r), gammaCorrect(color.g), gammaCorrect(color.b), 1.0);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) {
		parameters["Lwa"] = Parameter(image->getLogAverageLuminance(), "Lwa");
		parameters["Lwmax"] = Parameter(image->getMaximumLuminance(), "Lwmax");
	};

protected:
	virtual float map(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float Ldmax = parameters.at("Ldmax").value;
		float Lwa = parameters.at("Lwa").value;
		float Lwmax = parameters.at("Lwmax").value;
		float b = parameters.at("b").value;
		float start = parameters.at("start").value;
		float slope = parameters.at("slope").value;

		Lwa = exposure * Lwa / pow(1.f + b - 0.85f, 5.f);
		Lwmax = exposure * Lwmax / Lwa;
		value = exposure * value / Lwa;

		float exponent = std::log(b) / std::log(0.5f);
		float c1 = (0.01f * Ldmax) / std::log10(1.f + Lwmax);
		float c2 = std::log(1.f + value) / std::log(2.f + 8.f * (std::pow(value / Lwmax, exponent)));
		value = c1 * c2;

		if (value <= start) {
			return slope * value;
		}
		else {
			return std::pow(1.099f * value, 0.9f / gamma) - 0.099f;
		}
	}
};