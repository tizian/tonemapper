#pragma once

#include <tonemap.h>

class ReinhardDevlinOperator : public TonemapOperator {
public:
	ReinhardDevlinOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["m"] = Parameter(0.5f, 0.f, 1.f, "m", "Compression curve adjustment parameter");
		parameters["f"] = Parameter(1.f, 0.f, 1000.f, "f", "Intensity adjustment parameter");
		parameters["c"] = Parameter(0.f, 0.f, 1.f, "c", "Chromatic adaptation\nBlend between color channels and luminance.");
		parameters["a"] = Parameter(1.f, 0.f, 1.f, "a", "Light adaptation\nBlend between pixel intensity and average scene intensity.");

		name = "Reinhard-Devlin";
		description = "Reinhard-Devlin Mapping\n\nPropsed in \"Dynamic Range Reduction Inspired by Photoreceptor Physiology\" by Reinhard and Devlin 2005.";

		shader->init(
			"ReinhardDevlin",

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
			"uniform float m;\n"
			"uniform float f;\n"
			"uniform float c;\n"
			"uniform float a;\n"
			"uniform float Iav_r;\n"
			"uniform float Iav_g;\n"
			"uniform float Iav_b;\n"
			"uniform float Lav;\n"
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
			"float getLuminance(vec4 color) {\n"
			"	 return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;\n"
			"}\n"
			"\n"
			"float sigmaIa(float Ia, float Iav_a, float L) {\n"
			"	 float Ia_local = c * Ia + (1.0 - c) * L;\n"
			"	 float Ia_global = c * Iav_a + (1.0 - c) * exposure * Lav;\n"
			"	 float result = a * Ia_local + (1.0 - a) * Ia_global;\n"
			"	 return pow(f * result, m);\n"
			"}\n"
			"\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 float L = getLuminance(color);\n"
			"	 float sigmaIr = sigmaIa(color.r, exposure * Iav_r, L);\n"
			"	 float sigmaIg = sigmaIa(color.g, exposure * Iav_g, L);\n"
			"	 float sigmaIb = sigmaIa(color.b, exposure * Iav_b, L);\n"
			"	 color.r = color.r / (color.r + sigmaIr);\n"
			"	 color.g = color.g / (color.g + sigmaIg);\n"
			"	 color.b = color.b / (color.b + sigmaIb);\n"
			"	 color = clampedValue(color);\n"
			"    out_color = gammaCorrect(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		float Lmax = image->getMaximumLuminance();
		float Lav = image->getAverageLuminance();
		float Llav = image->getAverageLuminance();
		float Lmin = image->getMinimumLuminance();
		float k = (std::log(Lmax) - std::log(Llav)) / (std::log(Lmax) - std::log(Lmin));
		float m = 0.3f + 0.7f * std::pow(k, 1.4f);
		parameters.at("m").defaultValue = m;
		parameters.at("m").value = m;

		parameters["Iav_r"] = Parameter(image->getAverageIntensity().r(), "Iav_r");
		parameters["Iav_g"] = Parameter(image->getAverageIntensity().r(), "Iav_g");
		parameters["Iav_b"] = Parameter(image->getAverageIntensity().r(), "Iav_b");

		parameters["Lav"] = Parameter(Lav, "Lav");
	};

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float m = parameters.at("m").value;
		float f = parameters.at("f").value;
		float c = parameters.at("c").value;
		float a = parameters.at("a").value;
		float Iav_r = parameters.at("Iav_r").value;
		float Iav_g = parameters.at("Iav_g").value;
		float Iav_b = parameters.at("Iav_b").value;
		float Lav = parameters.at("Lav").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				Color3f out = map(color, exposure, gamma, m, f, c, a, Iav_r, Iav_g, Iav_b, Lav);
				dst[0] = (uint8_t) clamp(255.f * out.r(), 0.f, 255.f);
				dst[1] = (uint8_t) clamp(255.f * out.g(), 0.f, 255.f);
				dst[2] = (uint8_t) clamp(255.f * out.b(), 0.f, 255.f);
				dst += 3;
				*progress += delta;
			}
		}
	}

	float graph(float value) const override {
		float gamma = parameters.at("Gamma").value;
		float m = parameters.at("m").value;
		float f = parameters.at("f").value;
		float c = parameters.at("c").value;
		float a = parameters.at("a").value;
		float Iav_r = parameters.at("Iav_r").value;
		float Iav_g = parameters.at("Iav_g").value;
		float Iav_b = parameters.at("Iav_b").value;
		float Lav = parameters.at("Lav").value;

		return map(Color3f(value), 1.f, gamma, m, f, c, a, Iav_r, Iav_g, Iav_b, Lav).getLuminance();
	}

protected:
	Color3f map(const Color3f &col, float exposure, float gamma, float m, float f, float c, float a, float Iav_r, float Iav_g, float Iav_b, float Lav) const {
		Color3f color = exposure * col;
		float L = color.getLuminance();
		float sigmaIr = sigmaIa(color.r(), exposure * Iav_r, L, exposure * Lav, c, a, m, f);
		float sigmaIg = sigmaIa(color.g(), exposure * Iav_g, L, exposure * Lav, c, a, m, f);
		float sigmaIb = sigmaIa(color.b(), exposure * Iav_b, L, exposure * Lav, c, a, m, f);

		color.r() = color.r() / (color.r() + sigmaIr);
		color.g() = color.g() / (color.g() + sigmaIg);
		color.b() = color.b() / (color.b() + sigmaIb);

		return color.pow(1.f / gamma);
	}

	float sigmaIa(float Ia, float Iav_a, float L, float Lav, float c, float a, float m, float f) const {
		float Ia_local = c * Ia + (1.f - c) * L;
		float Ia_global = c * Iav_a + (1.f - c) * Lav;
		float result = a * Ia_local + (1.f - a) * Ia_global;
		return std::pow(f * result, m);
	}
};