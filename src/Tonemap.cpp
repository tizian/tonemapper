/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

#include <Image.h>
#ifdef TONEMAPPER_BUILD_GUI
    #include <nanogui/shader.h>
#endif

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
            gl_Position = vec4(2.0 * position.x - 1.0, 2.0 * position.y- 1.0, 0.0, 1.0);
            uv = vec2(position.x, 1.0 - position.y);
        }
    )glsl";
}

TonemapOperator::~TonemapOperator() {}

void TonemapOperator::preprocess(const Image *image) {}

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

void TonemapOperator::fromFile(const std::string &filename) {}

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

std::vector<std::string> TonemapOperator::orderedNames() {
    // Empty strings translate to spacing in the GUI
    std::vector<std::string> names = {
        "gamma",
        "srgb",
        "",
        "clamping",
        "maxdivision",
        "meanvalue",
        "exponential",
        "exponentiation",
        "logarithmic",
        "",
        "tumblin_rushmeier",
        "schlick",
        "ward",
        "ferwerda",
        "durand_dorsey",
        "reinhard",
        "reinhard_extended",
        "drago",
        "reinhard_devlin",
        "",
        "hejl_burgess_dawson",
        "aldridge",
        "hable",
        "hable_updated",
        "lottes",
        "day",
        "uchimura",
        "",
        "aces_hill",
        "aces_narkowicz",
        "aces_guy",
        "",
        "response_function_data_file",
    };

    /* In case new operators are added without adding them to the list above,
       append them at the end. */
    bool additional = false;
    for (auto const& kv: *TonemapOperator::constructors) {
        auto it = std::find(names.begin(), names.end(), kv.first);
        if (it == names.end()) {
            if (!additional) {
                names.push_back("");
                additional = true;
            }
            names.push_back(kv.first);
        }
    }

    return names;
}

} // Namespace tonemapper
