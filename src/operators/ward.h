#pragma once

#include <tonemap.h>

class WardOperator : public TonemapOperator {
public:
	WardOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["Ldmax"] = Parameter(100.f, 0.f, 200.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");

		name = "Ward";
		description = "Ward Mapping\n\nProposed in \"A contrast-based scalefactor for luminance display\" by Ward 1994.";

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
			"uniform float Ldmax;\n"
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
			"	 float Lda = Ldmax / 2.0;\n"
			"	 float m = pow((1.219 + pow(Lda, 0.4)) / (1.219 + pow(Lwa * exposure, 0.4)), 2.5);\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = m * color;\n"
			"	 color = color / Ldmax;\n"
			"	 color = clampedValue(color);\n"
			"    out_color = gammaCorrect(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		parameters["Lwa"] = Parameter(image->getLogAverageLuminance(), "Lwa");
	};

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float Lwa = parameters.at("Lwa").value;
		float Ldmax = parameters.at("Ldmax").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				float colorR = map(color.r(), exposure, gamma, Lwa, Ldmax);
				float colorG = map(color.g(), exposure, gamma, Lwa, Ldmax);
				float colorB = map(color.b(), exposure, gamma, Lwa, Ldmax);
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
		float Lwa = parameters.at("Lwa").value;
		float Ldmax = parameters.at("Ldmax").value;

		return map(value, 1.f, gamma, Lwa, Ldmax);
	}

protected:
	float map(float v, float exposure, float gamma, float Lwa, float Ldmax) const {
		float Lda = Ldmax / 2.f;
		float m = std::pow((1.219f + std::pow(Lda, 0.4f)) / (1.219f + std::pow(Lwa * exposure, 0.4f)), 2.5f);

		float value = exposure * v;
		value = m * value;
		value = value / Ldmax;
		return std::pow(value, 1.f / gamma);
	}
};