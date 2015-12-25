#pragma once

#include <tonemap.h>

class InsomniacOperator : public TonemapOperator {
public:
	InsomniacOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma");
		parameters["w"] = Parameter(10.f, 0.f, 20.f, "w");
		parameters["b"] = Parameter(0.1f, 0.f, 2.f, "b");
		parameters["t"] = Parameter(0.7f, 0.f, 1.f, "t");
		parameters["s"] = Parameter(0.8f, 0.f, 1.f, "s");
		parameters["c"] = Parameter(2.f, 0.f, 10.f, "c");

		name = "Insomniac (Day)";

		shader->init(
			"Insomniac",

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
			"uniform float w;\n"
			"uniform float b;\n"
			"uniform float t;\n"
			"uniform float s;\n"
			"uniform float c;\n"
			"uniform float Lavg;\n"
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
			"float tonemap(float x, float k) {\n"
			"	 if (x < c) {\n"
			"	 	 return k * (1.0-t)*(x-b) / (c - (1.0-t)*b - t*x);\n"
			"	 }\n"
			"	 else {\n"
			"	 	 return (1.0-k)*(x-c) / (s*x + (1.0-s)*w - c) + k;\n"
			"	 }\n"
			"}\n"
			"\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 color = color / Lavg;\n"
			"	 float k = (1.0-t)*(c-b) / ((1.0-s)*(w-c) + (1.0-t)*(c-b));\n"
			"	 color = vec4(tonemap(color.r, k), tonemap(color.g, k), tonemap(color.b, k), 1.0);\n"
			"	 color = gammaCorrect(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) {
		parameters["Lavg"] = Parameter(image->getAverageLuminance(), "Lavg");
	};

protected:
	virtual float map(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float Lavg = parameters.at("Lavg").value;
		float w = parameters.at("w").value;
		float b = parameters.at("b").value;
		float t = parameters.at("t").value;
		float s = parameters.at("s").value;
		float c = parameters.at("c").value;

		value = value / Lavg;

		float k = (1.f-t)*(c-b) / ((1.f-s)*(w-c) + (1.f-t)*(c-b));
		value = tonemap(value, k, c, w, b, s, t);

		return gammaCorrect(value, gamma);
	}

	float tonemap(float x, float k, float c, float w, float b, float s, float t) const {
		if (x < c) {
			return k * (1.f-t)*(x-b) / (c - (1.f-t)*b - t*x);
		}
		else {
			return (1.f-k)*(x-c) / (s*x + (1.f-s)*w - c) + k;
		}
	}
};