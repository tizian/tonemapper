/*
    src/reinhard_extended.h -- Extended Reinhard tonemapping operator
    
    Copyright (c) 2015 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license. 
*/

#pragma once

#include <tonemap.h>

class ExtendedReinhardOperator : public TonemapOperator {
public:
	ExtendedReinhardOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");

		name = "Reinhard (Extended)";
		description = "Extended Reinhard Mapping\n\nProposed in \"Photographic Tone Reproduction for Digital Images\" by Reinhard et al. 2002.\n(Extension that allows high luminances to burn out.)";

		shader->init(
			"ExtendedReinhard",

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
			"uniform float Lwhite;\n"
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
			"	 color = (color * (1.0 + color / (Lwhite * Lwhite))) / (1.0 + color);\n"
			"	 color = clampedValue(color);\n"
			"    out_color = gammaCorrect(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		float Lmin = image->getMinimumLuminance();
		float Lmax = image->getMaximumLuminance();

		parameters["Lwhite"] = Parameter(Lmax, Lmin, Lmax, "Lwhite", "Smallest luminance that will be mapped to pure white.");
	};

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float Lwhite = parameters.at("Lwhite").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				float colorR = map(color.r(), exposure, gamma, Lwhite);
				float colorG = map(color.g(), exposure, gamma, Lwhite);
				float colorB = map(color.b(), exposure, gamma, Lwhite);
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
		float Lwhite = parameters.at("Lwhite").value;

		return map(value, 1.f, gamma, Lwhite);
	}

protected:
	float map(float v, float exposure, float gamma, float Lwhite) const {
		float value = exposure * v;
		value = (value * (1.f + value / (Lwhite * Lwhite))) / (1.f + value);
		return std::pow(value, 1.f / gamma);
	}
};