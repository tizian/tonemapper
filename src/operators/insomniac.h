/*
    src/insomniac.h -- Insomniac tonemapping operator
    
    Copyright (c) 2015 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license. 
*/

#pragma once

#include <tonemap.h>

class InsomniacOperator : public TonemapOperator {
public:
	InsomniacOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["w"] = Parameter(10.f, 0.f, 20.f, "w", "White point\nMinimal value that is mapped to 1.");
		parameters["b"] = Parameter(0.1f, 0.f, 2.f, "b", "Black point\nMaximal value that is mapped to 0.");
		parameters["t"] = Parameter(0.7f, 0.f, 1.f, "t", "Toe strength\nAmount of blending between a straight-line curve and a purely asymptotic curve for the toe.");
		parameters["s"] = Parameter(0.8f, 0.f, 1.f, "s", "Shoulder strength\nAmount of blending between a straight-line curve and a purely asymptotic curve for the shoulder.");
		parameters["c"] = Parameter(2.f, 0.f, 10.f, "c", "Cross-over point\nPoint where the toe and shoulder are pieced together into a single curve.");

		name = "Insomniac (Day)";
		description = "Insomniac Mapping\n\nFrom \"An efficient and user-friendly tone mapping operator\" by Mike Day (Insomniac Games).";

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
			"	 color = clampedValue(color);\n"
			"    out_color = gammaCorrect(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		parameters["Lavg"] = Parameter(image->getAverageLuminance(), "Lavg");
	};

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float Lavg = parameters.at("Lavg").value;
		float w = parameters.at("w").value;
		float b = parameters.at("b").value;
		float t = parameters.at("t").value;
		float s = parameters.at("s").value;
		float c = parameters.at("c").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				float colorR = map(color.r(), exposure, gamma, Lavg, w, b, t, s, c);
				float colorG = map(color.g(), exposure, gamma, Lavg, w, b, t, s, c);
				float colorB = map(color.b(), exposure, gamma, Lavg, w, b, t, s, c);
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
		float Lavg = parameters.at("Lavg").value;
		float w = parameters.at("w").value;
		float b = parameters.at("b").value;
		float t = parameters.at("t").value;
		float s = parameters.at("s").value;
		float c = parameters.at("c").value;

		return map(value, 1.f, gamma, Lavg, w, b, t, s, c);
	}

protected:
	float map(float v, float exposure, float gamma, float Lavg, float w, float b, float t, float s, float c) const {
		float value = exposure * v;
		value = value / (exposure * Lavg);

		float k = (1.f-t)*(c-b) / ((1.f-s)*(w-c) + (1.f-t)*(c-b));

		if (value < c) {
			value = k * (1.f-t)*(value-b) / (c - (1.f-t)*b - t*value);
		}
		else {
			value = (1.f-k)*(value-c) / (s*value + (1.f-s)*w - c) + k;
		}

		return std::pow(value, 1.f / gamma);
	}
};