/*
    src/ferwerda.h -- Ferwerda tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class FerwerdaOperator : public TonemapOperator {
public:
	FerwerdaOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["Ldmax"] = Parameter(80.f, 0.f, 160.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");

		name = "Ferwerda";
		description = "Ferwerda Mapping\n\nProposed in \"A Model of Visual Adaptation for Realistic Image Synthesis\" by Ferwerda et al. 1996.";

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
			"float tp(float La) {\n"
			"	 float logLa = log(La)/log(10.0);\n"
			"	 float result;\n"
			"	 if (logLa <= -2.6) {\n"
			"	 	 result = -0.72;\n"
			"	 }\n"
			"	 else if (logLa >= 1.9) {\n"
			"	 	result = logLa - 1.255;\n"
			"	 }\n"
			"	 else {\n"
			"	 	 result = pow(0.249 * logLa + 0.65, 2.7) - 0.72;\n"
			"	 }\n"
			"	 return pow(10.0, result);\n"
			"}\n"
			"\n"
			"float ts(float La) {\n"
			"	 float logLa = log(La)/log(10.0);\n"
			"	 float result;\n"
			"	 if (logLa <= -3.94) {\n"
			"	 	 result = -2.86;\n"
			"	 }\n"
			"	 else if (logLa >= -1.44) {\n"
			"	 	result = logLa - 0.395;\n"
			"	 }\n"
			"	 else {\n"
			"	 	 result = pow(0.405 * logLa + 1.6, 2.18) -2.86;\n"
			"	 }\n"
			"	 return pow(10.0, result);\n"
			"}\n"
			"\n"
			"float getLuminance(vec4 color) {\n"
			"	 return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;\n"
			"}\n"
			"\n"
			"vec4 adjustColor(vec4 color, float L, float Ld) {\n"
			"	return Ld * color / L;\n"
			"}\n"
			"\n"
			"void main() {\n"
			"	 float Lda = Ldmax / 2.0;\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 float L = getLuminance(color);\n"
			"	 float mP = tp(Lda) / tp(exposure * Lwa);\n"
			"	 float mS = ts(Lda) / ts(exposure * Lwa);\n"
			"	 float k = (1.0 - (Lwa/2.0 - 0.01)/(10.0-0.01));\n"
			"	 k = clamp(k * k, 0.0, 1.0);\n"
			"	 float Ld = mP * L + k * mS * L;\n"
			"	 color = adjustColor(color, L, Ld);\n"
			"	 color = clampedValue(color);\n"
			"    out_color = gammaCorrect(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		parameters["Lwa"] = Parameter(image->getMaximumLuminance() / 2.f, "Lwa");
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
				float Lw = color.getLuminance();
				float Ld = map(Lw, exposure, Lwa, Ldmax);
				Color3f c = Ld * color / Lw;
				c = c.clampedValue();
				c = c.gammaCorrect(gamma);
				dst[0] = (uint8_t) (255.f * c.r());
				dst[1] = (uint8_t) (255.f * c.g());
				dst[2] = (uint8_t) (255.f * c.b());
				dst += 3;
				*progress += delta;
			}
		}
	}

	float graph(float value) const override {
		float gamma = parameters.at("Gamma").value;
		float Lwa = parameters.at("Lwa").value;
		float Ldmax = parameters.at("Ldmax").value;

		value = map(Color3f(value), 1.f, Lwa, Ldmax);
		value = clamp(value, 0.f, 1.f);
		value = std::pow(value, 1.f / gamma);
		return value;
	}

protected:
	float map(const Color3f &c, float exposure, float Lwa, float Ldmax) const {
		float Lda = Ldmax / 2.f;
		Color3f color = exposure * c;
		float L = color.getLuminance();

		float mP = tp(Lda) / tp(exposure * Lwa);
		float mS = ts(Lda) / ts(exposure * Lwa);

		float k = (1.f - (Lwa / 2.f - 0.01f) / (10.f - 0.01f));
		k = clamp(k * k, 0.f, 1.f);

		float Ld = mP * L + k * mS * L;

		return Ld;
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
		return std::pow(10.f, result);
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
		return std::pow(10.f, result);
	}
};