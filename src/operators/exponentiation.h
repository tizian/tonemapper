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

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float Lmax = parameters.at("Lmax").value;
		float p = parameters.at("p").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				float colorR = map(color.r(), exposure, gamma, Lmax, p);
				float colorG = map(color.g(), exposure, gamma, Lmax, p);
				float colorB = map(color.b(), exposure, gamma, Lmax, p);
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
		float Lmax = parameters.at("Lmax").value;
		float p = parameters.at("p").value;

		return map(value, 1.f, gamma, Lmax, p);
	}

protected:
	float map(float v, float exposure, float gamma, float Lmax, float p) const {
		float value = exposure * v;
		value = value / (exposure * Lmax);
		return std::pow(value, p / gamma);
	}
};