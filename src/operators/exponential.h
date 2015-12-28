#pragma once

#include <tonemap.h>

class ExponentialOperator : public TonemapOperator {
public:
	ExponentialOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["p"] = Parameter(1.f, 0.f, 20.f, "p", "Exponent numerator scale factor");
		parameters["q"] = Parameter(1.f, 0.f, 20.f, "q", "Exponent denominator scale factor");

		name = "Exponential";
		description = "Exponential Mapping\n\nProposed in \"A Comparison of techniques for the Transformation of Radiosity Values to Monitor Colors\" by Ferschin et al. 1994.";

		shader->init(
			"Exponential",

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
			"	 color = 1.0 - exp(-(color * p) / (exposure * Lavg * q));\n"
			"	 color = gammaCorrect(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		parameters["Lavg"] = Parameter(image->getAverageLuminance(), "Lavg");
	};

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float Lavg = parameters.at("Lavg").value;
		float p = parameters.at("p").value;
		float q = parameters.at("q").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				float colorR = map(color.r(), exposure, gamma, Lavg, p, q);
				float colorG = map(color.g(), exposure, gamma, Lavg, p, q);
				float colorB = map(color.b(), exposure, gamma, Lavg, p, q);
				dst[0] = (uint8_t) clamp(255.f * colorR, 0.f, 255.f);
				dst[1] = (uint8_t) clamp(255.f * colorG, 0.f, 255.f);
				dst[2] = (uint8_t) clamp(255.f * colorB, 0.f, 255.f);
				dst += 3;
				*progress += delta;
			}
		}
	}

	float graph(float value) const override {
		float gamma = parameters.at("Gamma").value;
		float Lavg = parameters.at("Lavg").value;
		float p = parameters.at("p").value;
		float q = parameters.at("q").value;

		return map(value, 1.f, gamma, Lavg, p, q);
	}

protected:
	float map(float v, float exposure, float gamma, float Lavg, float p, float q) const {
		float value = exposure * v;
		value = 1.f - std::exp(-(value * p) / (exposure * Lavg * q));
		return std::pow(value, 1.f / gamma);
	}
};