#pragma once

#include <global.h>
#include <nanogui/glutil.h>

struct Parameter {
	float value;
	float defaultValue;
	float minValue;
	float maxValue;
	std::string uniform;
	std::string description;
	bool constant;

	Parameter() {}
	
	Parameter(float defaultValue, float minValue, float maxValue, const std::string &uniform, const std::string &description)
		: value(defaultValue), defaultValue(defaultValue), minValue(minValue), maxValue(maxValue),
		uniform(uniform), description(description), constant(false) {}
	Parameter(float value, const std::string &uniform, const std::string &description)
		: value(value), uniform(uniform), description(description), constant(true) {}

		Parameter(float defaultValue, float minValue, float maxValue, const std::string &uniform)
		: value(defaultValue), defaultValue(defaultValue), minValue(minValue), maxValue(maxValue),
		uniform(uniform), description(""), constant(false) {}
	Parameter(float value, const std::string &uniform)
		: value(value), uniform(uniform), description(""), constant(true) {}
};

typedef std::map<std::string, Parameter> ParameterMap;

class Image;

class TonemapOperator {
public:
	std::string 		name;
	std::string 		description;
	ParameterMap 		parameters;
	nanogui::GLShader  *shader = nullptr;
	
	TonemapOperator() {
		parameters = ParameterMap();
		shader = new nanogui::GLShader();
		description = "<no description>";
		name = "<no name>";
	}

	virtual ~TonemapOperator() {
		delete shader;
	}

	std::string getString() const { return name; }

	virtual void setParameters(const Image *image) {}
	virtual void process(const Image *image, uint8_t *dst, float exposure, float *progress) const {}
	virtual float graph(float value) const { return 0.f; }
};