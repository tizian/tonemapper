/*
    src/drago.h -- Drago tonemapping operator
    
    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license. 
*/

#pragma once

#include <tonemap.h>

class DragoOperator : public TonemapOperator {
public:
	DragoOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["slope"] = Parameter(4.5f, 0.f, 10.f, "slope", "Additional Gamma correction parameter:\nElevation ratio of the line passing by the origin and tangent to the curve.");
		parameters["start"] = Parameter(0.018f, 0.f, 2.f, "start", "Additional Gamma correction parameter:\nAbscissa at the point of tangency.");
		parameters["Ldmax"] = Parameter(100.f, 0.f, 200.f, "Ldmax", "Maximum luminance capability of the display (cd/m^2)");
		parameters["b"] = Parameter(0.85f, 0.f, 1.f, "b", "Bias function parameter");

		name = "Drago";
		description = "Drago Mapping\n\nPropsed in \"Adaptive Logarithmic Mapping For Displaying High Contrast Scenes\" by Drago et al. 2003.";

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
			"float gammaCorrect(float Ld) {\n"
			"	 if (Ld <= start) {\n"
			"	 	 return slope * Ld;\n"
			"	 }\n"
			"	 else {\n"
			"	 	 return pow(1.099 * Ld, 0.9/gamma) - 0.099;\n"
			"	 }\n"
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
			"	 float LwaP = exposure * Lwa / pow(1.0 + b - 0.85, 5);\n"
			"	 float LwmaxP = exposure * Lwmax / LwaP;\n"
			"    vec4 color = exposure * texture(source, uv) / LwaP;\n"
			"	 float L = getLuminance(color);\n"
			"	 float exponent = log(b) / log(0.5);\n"
			"	 float c1 = (0.01 * Ldmax) / (log(1 + LwmaxP)/log(10.0));\n"
			"	 float c2 = log(L + 1) / log(2.0 + 8 * (pow(L / LwmaxP, exponent)));\n"
			"	 float Ld = c1 * c2;\n"
			"	 color = adjustColor(color, L, Ld);\n"
			"	 color = clampedValue(color);\n"
			"	 out_color = vec4(gammaCorrect(color.r), gammaCorrect(color.g), gammaCorrect(color.b), 1.0);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		parameters["Lwa"] = Parameter(image->getLogAverageLuminance(), "Lwa");
		parameters["Lwmax"] = Parameter(image->getMaximumLuminance(), "Lwmax");
	};

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float Ldmax = parameters.at("Ldmax").value;
		float Lwa = parameters.at("Lwa").value;
		float Lwmax = parameters.at("Lwmax").value;
		float b = parameters.at("b").value;
		float start = parameters.at("start").value;
		float slope = parameters.at("slope").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				float Lw = color.getLuminance();
				float Ld = map(Lw, exposure, Ldmax, Lwa, Lwmax, b);
				Color3f c = Ld * color / Lw;
				c = c.clampedValue();
				c = Color3f(gammaCorrect(c.r(), gamma, start, slope),
							gammaCorrect(c.g(), gamma, start, slope),
							gammaCorrect(c.b(), gamma, start, slope));
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
		float Ldmax = parameters.at("Ldmax").value;
		float Lwa = parameters.at("Lwa").value;
		float Lwmax = parameters.at("Lwmax").value;
		float b = parameters.at("b").value;
		float start = parameters.at("start").value;
		float slope = parameters.at("slope").value;

		value = map(value, 1.f, Ldmax, Lwa, Lwmax, b);
		value = clamp(value, 0.f, 1.f);
		value = gammaCorrect(value, gamma, start, slope);
		return value;
	}

protected:
	float map(float Lw, float exposure, float Ldmax, float Lwa, float Lwmax, float b) const {
		Lwa = exposure * Lwa / std::pow(1.f + b - 0.85f, 5.f);
		Lwmax = exposure * Lwmax / Lwa;
		float L = exposure * Lw / Lwa;
		
		float exponent = std::log(b) / std::log(0.5f);
		float c1 = (0.01f * Ldmax) / std::log10(1.f + Lwmax);
		float c2 = std::log(1.f + L) / std::log(2.f + 8.f * (std::pow(L / Lwmax, exponent)));
		float Ld = c1 * c2;

		return Ld;
	}

	float gammaCorrect(float Ld, float gamma, float start, float slope) const {
		if (Ld <= start) {
			return slope * Ld;
		}
		else {
			return std::pow(1.099 * Ld, 0.9/gamma) - 0.099;
		}
	}
};