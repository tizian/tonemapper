#pragma once

#include <global.h>
#include <nanogui/glutil.h>

struct Parameter {
	float value;
	float defaultValue;
	float minValue;
	float maxValue;
	std::string uniform;

	Parameter() {}
	Parameter(float defaultValue, float minValue, float maxValue, const std::string &uniform)
		: value(defaultValue), defaultValue(defaultValue), minValue(minValue), maxValue(maxValue), uniform(uniform) {}
};

typedef std::map<std::string, Parameter> ParameterMap;

class ToneMappingOperator {
public:
	ToneMappingOperator() {
		parameters = ParameterMap();
		parameters["Exposure"] = Parameter(1.f, 0.f, 10.f, "exposure");

		shader = new nanogui::GLShader();
	}

	virtual ~ToneMappingOperator() {
		delete shader;
	}

	ParameterMap parameters;
	nanogui::GLShader *shader = nullptr;
};