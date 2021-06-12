#include <Tonemap.h>

#include <Image.h>

namespace tonemapper {

TonemapOperator::TonemapOperator() {
    parameters = ParameterMap();
}

// Process each pixel in the image
void TonemapOperator::process(const Image *input, Image *output, float exposure, float *progress) const {
    if (progress) *progress = 0.f;
    float delta = 1.f / (input->getWidth() * input->getHeight());

    for (size_t i = 0; i < input->getHeight(); ++i) {
        for (size_t j = 0; j < input->getWidth(); ++j) {
            const Color3f &color = exposure * input->ref(i, j);
            output->ref(i, j) = map(color);
            if (progress) *progress += delta;
        }
    }
}

std::map<std::string, TonemapOperator::Constructor> *TonemapOperator::constructors = nullptr;

TonemapOperator *TonemapOperator::create(const std::string &name) {
    if (!constructors || constructors->find(name) == constructors->end()) {
        ERROR("A constructor for class \"%s\" could not be found!", name);
    }
    TonemapOperator *op = (*constructors)[name]();
    return op;
}

void TonemapOperator::registerOperator(const std::string &name, const Constructor &constr) {
    if (!constructors) {
        constructors = new std::map<std::string, TonemapOperator::Constructor>();
    }
    (*constructors)[name] = constr;
}

} // Namespace tonemapper
