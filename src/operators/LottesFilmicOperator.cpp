#include <Tonemap.h>

namespace tonemapper {

class LottesFilmicOperator : public TonemapOperator {
public:
    LottesFilmicOperator() : TonemapOperator() {
        name = "Lottes Filmic";
        description = R"(Filmic curve by Timothy Lottes, described in his GDC
            talk "Advanced Techniques and Optimization of HDR Color Pipelines".
            Also known as the "AMD curve".)";

        /* See also the following shadertoy by Romain Guy:
           https://www.shadertoy.com/view/llXyWr */
        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float contrast;
            uniform float shoulder;
            uniform float hdrMax;
            uniform float midIn;
            uniform float midOut;

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply curve directly on color input
                float a = contrast,
                      d = shoulder,
                      b = (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
                          ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut),
                      c = (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
                          ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
                vec3 Cout = pow(Cin, vec3(a)) / (pow(Cin, vec3(a * d)) * b + c);

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"]    = Parameter(2.2f,   0.f,   10.f, "gamma",    "Gamma correction value.");
        parameters["contrast"] = Parameter(1.6f,   1.f,   2.f,  "contrast", "Contrast control.");
        parameters["shoulder"] = Parameter(0.977f, 0.01f, 2.f,  "shoulder", "Shoulder control.");
        parameters["hdrMax"]   = Parameter(8.f,    1.f,   10.f, "hdrMax",   "Maximum HDR value.");
        parameters["midIn"]    = Parameter(0.18f,  0.f,   1.f,  "midIn",    "Input mid-level.");
        parameters["midOut"]   = Parameter(0.267f, 0.f,   1.f,  "midOut",   "Output mid-level");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma    = parameters.at("gamma").value,
              contrast = parameters.at("contrast").value,
              shoulder = parameters.at("shoulder").value,
              hdrMax   = parameters.at("hdrMax").value,
              midIn    = parameters.at("midIn").value,
              midOut   = parameters.at("midOut").value;

        // Fetch color
        Color3f Cin = exposure * color;

        // Apply curve directly on color input
        float a = contrast,
              d = shoulder,
              b = (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
                  ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut),
              c = (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
                  ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
        Color3f Cout = pow(Cin, a) / (pow(Cin, a * d) * b + c);

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(LottesFilmicOperator, "lottes");

} // Namespace tonemapper
