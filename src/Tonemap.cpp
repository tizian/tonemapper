#include <Tonemap.h>

#include <Image.h>
#include <nanogui/shader.h>

namespace tonemapper {

TonemapOperator::TonemapOperator() {
    parameters = ParameterMap();
    name = "<no name>";
    description = "<no description>";

    vertexShader = R"glsl(
        #version 330
        in vec2 position;
        out vec2 uv;
        void main() {
            gl_Position = vec4(position.x*2-1, position.y*2-1, 0.0, 1.0);
            uv = vec2(position.x, 1-position.y);
        }
    )glsl";
}

// Process each pixel in the image
void TonemapOperator::process(const Image *input, Image *output, float exposure, float *progress) const {
    if (progress) *progress = 0.f;
    float delta = 1.f / (input->getWidth() * input->getHeight());

    for (size_t i = 0; i < input->getHeight(); ++i) {
        for (size_t j = 0; j < input->getWidth(); ++j) {
            const Color3f &color = input->ref(i, j);
            output->ref(i, j) = map(color, exposure);
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
