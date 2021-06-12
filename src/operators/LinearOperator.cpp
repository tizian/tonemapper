#include <Tonemap.h>

namespace tonemapper {

class LinearOperator : public TonemapOperator {
public:
    LinearOperator() {
        name = "Linear";
        description = "Linear operator.";

        parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
    }

    virtual Color3f map(const Color3f &c) const {
        float gamma = parameters.at("Gamma").value;
        return pow(c, 1.f / gamma);
    }
};

REGISTER_OPERATOR(LinearOperator, "linear");

} // Namespace tonemapper
