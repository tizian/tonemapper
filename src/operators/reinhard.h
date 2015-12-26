#pragma once

#include <tonemap.h>

class ReinhardOperator : public TonemapOperator {
public:
	ReinhardOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");

		name = "Reinhard";
		description = "Reinhard Mapping\n\nProposed in \"Photographic Tone Reproduction for Digital Images\" by Reinhard et al. 2002.\n(Simple operator)";

		shader->init(
			"Reinhard",

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
			"	 color = color / (1.0 + color);\n"
			"	 color = gammaCorrect(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

protected:
	virtual float map(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		
		value *= exposure;
		value = value / (1.f + value);
		return gammaCorrect(value, gamma);
	}
};