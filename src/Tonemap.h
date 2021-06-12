#pragma once

#include <Global.h>
#include <Color.h>
#include <map>

namespace tonemapper {

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
class GLShader;

class TonemapOperator {
public:
    TonemapOperator();
    virtual ~TonemapOperator() {}

    // Set some of the operator parameters based on image data (e.g. mean color)
    virtual void preprocess(const Image *image) {}

    // Process each pixel in the image
    void process(const Image *input, Image *output, float exposure, float *progress=nullptr) const;

    // Actual tonemapping operator
    virtual Color3f map(const Color3f &c) const = 0;

public:
    ParameterMap        parameters;
    std::string         name;
    std::string         description;
    GLShader  *shader = nullptr;

public:
    typedef std::function<TonemapOperator *()> Constructor;

    static TonemapOperator *create(const std::string &name);
    static void registerOperator(const std::string &name, const Constructor &constr);
    static std::map<std::string, Constructor> *constructors;
};

#define REGISTER_OPERATOR(cls, name) \
    cls *cls##create() { \
        return new cls(); \
    } \
    static struct cls##_{ \
        cls##_() { \
            TonemapOperator::registerOperator(name, cls##create); \
        } \
    } cls##__STATIC_;


} // Namespace tonemapper
