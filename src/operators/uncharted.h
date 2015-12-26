#pragma once

#include <tonemap.h>

class Uncharted2Operator : public TonemapOperator {
public:
	Uncharted2Operator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["A"] = Parameter(0.22f, 0.f, 1.f, "A", "Shoulder strength curve parameter");
		parameters["B"] = Parameter(0.3f, 0.f, 1.f, "B", "Linear strength curve parameter");
		parameters["C"] = Parameter(0.1f, 0.f, 1.f, "C", "Linear angle curve parameter");
		parameters["D"] = Parameter(0.2f, 0.f, 1.f, "D", "Toe strength curve parameter");
		parameters["E"] = Parameter(0.01f, 0.f, 1.f, "E", "Toe numerator curve parameter");
		parameters["F"] = Parameter(0.3f, 0.f, 1.f, "F", "Toe denominator curve parameter");
		parameters["W"] = Parameter(11.2f, 0.f, 20.f, "W", "White point\nMinimal value that is mapped to 1.");

		name = "Uncharted 2 (Hable)";
		description = "Uncharted 2 Mapping\n\nBy John Hable.";

		shader->init(
			"Uncharted2",

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
			"uniform float A;\n"
			"uniform float B;\n"
			"uniform float C;\n"
			"uniform float D;\n"
			"uniform float E;\n"
			"uniform float F;\n"
			"uniform float W;\n"
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
			"vec4 tonemap(vec4 x) {\n"
			"	 return ((x * (A*x + C*B) + D*E) / (x * (A*x+B) + D*F)) - E/F;\n"
			"}\n"
			"\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 float exposureBias = 2.0;\n"
			"	 vec4 curr = tonemap(exposureBias * color);\n"
			"	 vec4 whiteScale = 1.0 / tonemap(vec4(W));\n"
			"	 color = curr * whiteScale;\n"
			"	 color = gammaCorrect(color);\n"
			"    out_color = clampedValue(color);\n"
			"}"
		);
	}

protected:
	virtual float map(float value, float exposure) const override {
		float gamma = parameters.at("Gamma").value;
		float A = parameters.at("A").value;
		float B = parameters.at("B").value;
		float C = parameters.at("C").value;
		float D = parameters.at("D").value;
		float E = parameters.at("E").value;
		float F = parameters.at("F").value;
		float W = parameters.at("W").value;
		float exposureBias = 2.f;

		value *= exposure;
		float curr = tonemap(exposureBias * value, A, B, C, D, E, F);
		float whiteScale = 1.f / tonemap(W, A, B, C, D, E, F);
		value = curr * whiteScale;

		return gammaCorrect(value, gamma);
	}

	float tonemap(float x, float A, float B, float C, float D, float E, float F) const {
		return ((x * (A*x + C*B) + D*E) / (x * (A*x+B) + D*F)) - E/F;
	}
};