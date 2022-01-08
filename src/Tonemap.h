/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <Global.h>
#include <Color.h>
#include <Image.h>
#include <map>
#include <functional>

namespace tonemapper {

struct Parameter {
    float value;
    float defaultValue;
    float minValue;
    float maxValue;
    std::string uniform;
    std::string description;
    bool constant;

    Parameter()
        : value(0.f), defaultValue(0.f), minValue(0.f), maxValue(0.f), uniform(""), description(""), constant(true) {}

    Parameter(float defaultValue, float minValue, float maxValue, const std::string &uniform, const std::string &description)
        : value(defaultValue), defaultValue(defaultValue), minValue(minValue), maxValue(maxValue), uniform(uniform), description(description), constant(false) {}

    Parameter(float value, const std::string &uniform, const std::string &description)
        : value(value), defaultValue(value), minValue(value), maxValue(value), uniform(uniform), description(description), constant(true) {}

    Parameter(float defaultValue, float minValue, float maxValue, const std::string &uniform)
        : value(defaultValue), defaultValue(defaultValue), minValue(minValue), maxValue(maxValue), uniform(uniform), description(""), constant(false) {}

    Parameter(float value, const std::string &uniform)
        : value(value), defaultValue(value), minValue(value), maxValue(value), uniform(uniform), description(""), constant(true) {}
};

typedef std::map<std::string, Parameter> ParameterMap;

class Image;

class TonemapOperator {
public:
    TonemapOperator();
    virtual ~TonemapOperator();

    // Set some of the operator parameters based on image data (e.g. mean color)
    virtual void preprocess(const Image *image);

    // Process each pixel in the image
    void process(const Image *input, Image *output, float exposure, float *progress=nullptr) const;

    // Actual tonemapping operator
    virtual Color3f map(const Color3f &c, float exposure) const = 0;

    virtual void fromFile(const std::string &filename);

public:
    ParameterMap parameters;
    std::string  name;
    std::string  description;
    std::string  vertexShader;
    std::string  fragmentShader;

    bool dataDriven = false;
    std::vector<float> irradiance;
    std::vector<float> values[3];

public:
    typedef std::function<TonemapOperator *()> Constructor;

    static TonemapOperator *create(const std::string &name);
    static void registerOperator(const std::string &name, const Constructor &constr);
    static std::map<std::string, Constructor> *constructors;

    static std::vector<std::string> orderedNames();
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
