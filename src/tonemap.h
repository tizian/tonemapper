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
		uniform(uniform), description("blub"), constant(false) {}
	Parameter(float value, const std::string &uniform)
		: value(value), uniform(uniform), description("blub"), constant(true) {}
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

	virtual Color3f map(const Color3f &color, float exposure = 1.f) const {
		return Color3f(map(color.r(), exposure), map(color.g(), exposure), map(color.b(), exposure));
	}

	virtual void setParameters(const Image *image) {}

	virtual std::string getString() const {
		return name;
	}

protected:
	virtual float map(float value, float exposure = 1.f) const { 
		return exposure * value;
	}
};