#pragma once

#include <tonemap.h>

class FerwerdaOperator : public TonemapOperator {
public:
	FerwerdaOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");
		parameters["max Ld"] = Parameter(80.f, 0.f, 160.f, "maxLd");

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
			"float s(vec4 color) {\n"
			"	 vec3 xyz;\n"
			"	 xyz.x = color.r * 0.4124 + color.g * 0.3576 + color.b * 0.1805;\n"
			"	 xyz.y = color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722;\n"
			"	 xyz.z = color.r * 0.0193 + color.g * 0.1192 + color.b * 0.9505;\n"
			"	 return -0.702 * xyz.x + 1.039 * xyz.y + 0.433 * xyz.z;\n"
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
			"void main() {\n"
			"	 float Lda = maxLd / 2.0;\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 float mP = tp(Lda) / tp(exposure * Lwa);\n"
			"	 float mS = ts(Lda) / ts(exposure * Lwa);\n"
			"	 float k = (1.0 - (Lwa/2.0 - 0.01)/(10.0-0.01));\n"
			"	 k = clamp(k * k, 0.0, 1.0);\n"
			"	 float sw = s(color);\n"
			"	 color = mP * color + k * mS * sw;\n"
			"	 color = color / maxLd;\n"
			"	 color = gammaCorrect(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) {
		parameters["Lwa"] = Parameter(image->getMaximumLuminance() / 2.f, "Lwa");
	};

	virtual Color3f map(const Color3f &color, float exposure = 1.f) const {
		float gamma = parameters.at("Gamma").value;
		float Lwa = parameters.at("Lwa").value;
		float maxLd = parameters.at("max Ld").value;
		float Lda = maxLd / 2.f;

		Color3f c = exposure * color;

		float mP = tp(Lda) / tp(exposure * Lwa);
		float mS = ts(Lda) / ts(exposure * Lwa);

		float k = (1.f - (Lwa/2.f - 0.01f) / (10.f - 0.01f));
		k = clamp(k * k, 0.f, 1.f);

		float sw = s(c);

		c = mP * c + k * mS * Color3f(sw);
		c = c / maxLd;

		return Color3f(gammaCorrect(c.r(), gamma), gammaCorrect(c.g(), gamma), gammaCorrect(c.b(), gamma));
	}

protected:
	inline float s(const Color3f &color) const {
		Color3f xyz;
		xyz.r() = color.r() * 0.4124f + color.g() * 0.3576f + color.b() * 0.1805f;
		xyz.g() = color.r() * 0.2126f + color.g() * 0.7152f + color.b() * 0.0722f;
		xyz.b() = color.r() * 0.0193f + color.g() * 0.1192f + color.b() * 0.9505f;
		return -0.702f * xyz.r() + 1.039f * xyz.g() + 0.433f * xyz.b();
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