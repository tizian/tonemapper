/*
    Copyright (c) 2022 Tizian Zeltner

    tonemapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class ReinhardDevlinOperator : public TonemapOperator {
public:
    ReinhardDevlinOperator() : TonemapOperator() {
        name = "Reinhard Devlin";
        description = R"(Mapping proposed in "Dynamic Range Reduction Inspired
            by Photoreceptor Physiology" by Reinhard and Devlin 2005.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float f;
            uniform float c;
            uniform float a;
            uniform float m;
            uniform float CmeanR;
            uniform float CmeanG;
            uniform float CmeanB;
            uniform float Lavg;

            float luminance(vec3 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            void main() {
                // Fetch color
                vec3 Cin = exposure * texture(source, uv).rgb;

                // Apply exposure scale to parameters
                float Lavg_   = exposure * Lavg,
                      CmeanR_ = exposure * CmeanR,
                      CmeanG_ = exposure * CmeanG,
                      CmeanB_ = exposure * CmeanB;
                vec3 Cmean = vec3(CmeanR_, CmeanG_, CmeanB_);


                // Apply tonemapping curve, separately for each channel
                float L     = luminance(Cin),
                      f_    = exp(-f);
                vec3  Il    = c * Cin   + (1.0 - c) * L,
                      Ig    = c * Cmean + (1.0 - c) * Lavg_,
                      Ia    = a * Il    + (1.0 - c) * Ig,
                      Cout  = Cin / (Cin + pow(f_ * Ia, vec3(m)));

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value.");
        parameters["f"]     = Parameter(0.f, -8.f, 8.f,  "f",     "Intensity adjustment parameter.");
        parameters["c"]     = Parameter(0.f,  0.f, 1.f,  "c",     "Chromatic adaptation.");
        parameters["a"]     = Parameter(1.f,  0.f, 1.f,  "a",     "Light adaptation.");
    }

    void preprocess(const Image *image) override {
        Color3f Cmean = image->getMean();
        parameters["CmeanR"] = Parameter(Cmean.r(), "CmeanR");
        parameters["CmeanG"] = Parameter(Cmean.g(), "CmeanG");
        parameters["CmeanB"] = Parameter(Cmean.b(), "CmeanB");

        float min      = image->getMinimumLuminance(),
              max      = image->getMaximumLuminance(),
              k        = (std::log(max) - image->getLogMeanLuminance()) / (std::log(max) - std::log(min)),
              mDefault = 0.3f + 0.7f * std::pow(k, 1.4f);
        parameters["m"] = Parameter(mDefault, 0.3f, 1.f, "m", "Contrast parameter.");

        parameters["Lavg"] = Parameter(image->getMeanLuminance(), "Lavg");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma  = parameters.at("gamma").value,
              f      = parameters.at("f").value,
              c      = parameters.at("c").value,
              a      = parameters.at("a").value,
              m      = parameters.at("m").value,
              CmeanR = parameters.at("CmeanR").value,
              CmeanG = parameters.at("CmeanG").value,
              CmeanB = parameters.at("CmeanB").value,
              Lavg   = parameters.at("Lavg").value;

        // Fetch color and convert to luminance
        Color3f Cin = exposure * color;

        // Apply exposure scale to parameters
        Lavg   *= exposure;
        CmeanR *= exposure;
        CmeanG *= exposure;
        CmeanB *= exposure;
        Color3f Cmean(CmeanR, CmeanG, CmeanB);

        // Apply tonemapping curve, separately for each channel
        f = std::exp(-f);
        float    L     = luminance(Cin);
        Color3f  Il    = c * Cin   + (1.f - c) * L,
                 Ig    = c * Cmean + (1.f - c) * Lavg,
                 Ia    = a * Il    + (1.f - c) * Ig,
                 Cout  = Cin / (Cin + pow(f * Ia, m));

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(ReinhardDevlinOperator, "reinhard_devlin");

} // Namespace tonemapper
