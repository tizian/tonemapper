#pragma once

#include <global.h>
#include <nanogui/glutil.h>

struct Parameter {
	float value;
	float defaultValue;
	float minValue;
	float maxValue;
	std::string uniform;
	bool constant;

	Parameter() {}
	Parameter(float defaultValue, float minValue, float maxValue, const std::string &uniform)
		: value(defaultValue), defaultValue(defaultValue), minValue(minValue), maxValue(maxValue), uniform(uniform), constant(false) {}
	Parameter(float value, const std::string &uniform)
		: value(value), uniform(uniform), constant(true) {}
};

typedef std::map<std::string, Parameter> ParameterMap;

class Image;

class TonemapOperator {
public:
	TonemapOperator() {
		parameters = ParameterMap();
		shader = new nanogui::GLShader();
	}

	virtual ~TonemapOperator() {
		delete shader;
	}

	virtual float correct(float value, float exposure = 1.f) const = 0;

	virtual void setParameters(const Image *image) {}

	virtual std::string getString() const {
		return name;
	}

	ParameterMap parameters;
	nanogui::GLShader *shader = nullptr;
	std::string name;
};