#pragma once

#include <tonemap.h>

class SchlickOperator : public TonemapOperator {
public:
	SchlickOperator() : TonemapOperator() {
		parameters["p"] = Parameter(200.f, 1.f, 1000.f, "p");

		name = "Schlick";

		shader->init(
			"Schlick",

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
			"uniform float maxLum;\n"
			"uniform float p;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = p * color / (p * color - color + exposure * maxLum);\n"
			"    out_color = vec4(color.r, color.g, color.b, 1);\n"
			"}"
		);
	}

	virtual float correct(float value, float exposure) const override {
		float maxLum = parameters.at("maxLum").value;
		float p = parameters.at("p").value;
		value *= exposure;
		value = p * value / (p * value - value + exposure * maxLum);
		return value;
	}

	virtual void setParameters(const Image *image) {
		parameters["maxLum"] = Parameter(image->getMaximumLuminance(), "maxLum");
	};
};