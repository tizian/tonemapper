/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

#include <fstream>

namespace tonemapper {

class ResponseFunctionDataFileOperator : public TonemapOperator {
public:
    ResponseFunctionDataFileOperator() : TonemapOperator() {
        name = "Response Function Data File";
        description = R"(Use data-driven curves specified via separate text
            files)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform sampler2D dataR;
            uniform sampler2D dataG;
            uniform sampler2D dataB;
            uniform float exposure;
            uniform float W;

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve
                vec3 Cout = vec3(texture(dataR, vec2(Cin.r / W, 0.0)).r,
                                 texture(dataG, vec2(Cin.g / W, 0.0)).r,
                                 texture(dataB, vec2(Cin.b / W, 0.0)).r);

                /* Gamma correction is already included in the mapping above
                   and only clamping is applied. */
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["W"] = Parameter(1.f, 1e-5f, 10.f, "W", "White point.");

        dataDriven = true;
    }

    Color3f map(const Color3f &color, float exposure) const override {
        if (irradiance.size() == 0) {
            return Color3f(0.f);
        }

        auto eval = [this](const Color3f &c) {
            Color3f result(0.f);
            for (size_t i = 0; i < 3; ++i) {
                float x = c[i];
                if (x < 0.f) {
                    continue;
                }

                if (x >= 1.f) {
                    result[i] = 1.f;
                }

                int idx = findInterval(irradiance.size(), [&](size_t idx) {
                    return irradiance[idx] <= x;
                });
                float x0 = irradiance[idx],
                      x1 = irradiance[idx + 1],
                      y0 = values[i][idx],
                      y1 = values[i][idx + 1];

                x = (x - x0) / (x1 - x0);
                result[i] = x*(y1 - y0) + y0;
            }
            return result;
        };

        // Fetch parameters
        float W = parameters.at("W").value;

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve
        Color3f Cout = eval(Cin / W);

        /* Gamma correction is already included in the mapping above
           and only clamping is applied. */
        return clamp(Cout, 0.f, 1.f);
    }

    void fromFile(const std::string &filename) override {
        std::filesystem::path path(filename);
        PRINT_("Read camera response function %s ..", path.filename());

        irradiance.clear();
        values[0].clear();
        values[1].clear();
        values[2].clear();

        std::ifstream is(filename);
        if (is.bad() || is.fail()) {
            PRINT("");
            WARN("ResponseFunctionDataOperator::fromFile: could not open data file %s.", path.filename());
            return;
        }

        std::string line;
        while (std::getline(is, line)) {
            if (line.length() == 0 || line[0] == '#') {
                continue;
            }
            std::istringstream iss(line);

            float irr, r, g, b;
            if (!(iss >> irr >> r >> g >> b)) {
                break;
            }
            irradiance.push_back(irr);
            values[0].push_back(r);
            values[1].push_back(g);
            values[2].push_back(b);
        }

        if (irradiance.size() == 0) {
            PRINT("");
            WARN("ResponseFunctionDataOperator::fromFile: could not read any data in file %s.", path.filename());
        } else {
            PRINT(" done.");
        }
    }
};

REGISTER_OPERATOR(ResponseFunctionDataFileOperator, "response_function_data_file");

} // Namespace tonemapper
