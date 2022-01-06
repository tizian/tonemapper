#pragma once

#define YEAR    "2022"
#define VERSION "2.0.dev"

#include <tinyformat.h>
#include <string>
#include <vector>

#define TERM_COLOR_RED    "\x1B[31m"
#define TERM_COLOR_YELLOW "\x1B[33m"
#define TERM_COLOR_WHITE  "\x1B[37m"

#define LOG(str, ...)   std::cout << tfm::format(str "\n", ##__VA_ARGS__)
#define PRINT(str, ...) std::cout << tfm::format(str "\n", ##__VA_ARGS__)
#define PRINT_(str, ...) std::cout << tfm::format(str, ##__VA_ARGS__)
#define INFO(str, ...)  std::cout << tfm::format("%s(%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define WARN(str, ...)  std::cout << tfm::format(TERM_COLOR_YELLOW  str "\n" TERM_COLOR_WHITE, ##__VA_ARGS__)
#define WARN_VERBOSE(str, ...)  std::cout << tfm::format(TERM_COLOR_YELLOW "%s(%d): " str "\n" TERM_COLOR_WHITE, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERROR(str, ...) throw std::runtime_error(tfm::format(TERM_COLOR_RED "\nError - %s(%d): " str "\n" TERM_COLOR_WHITE, __FILE__, __LINE__, ##__VA_ARGS__))

#define VARLOG(x)           (std::cout << #x << ": " << (x) << std::endl)
#define VARLOG2(x, y)       (std::cout << #x << ": " << (x) << ", " << #y << ": " << (y) << std::endl)
#define VARLOG3(x, y, z)    (std::cout << #x << ": " << (x) << ", " << #y << ": " << (y) << ", " << #z << ": " << (z) << std::endl)
#define VARLOG4(x, y, z, w) (std::cout << #x << ": " << (x) << ", " << #y << ": " << (y) << ", " << #z << ": " << (z) << ", " << #w << ": " << (w) << std::endl)

namespace tonemapper {

enum class ExposureMode {
    Value = 0,
    Key,
    Auto
};

inline std::string fileExtension(const std::string &filename) {
    size_t idx = filename.rfind('.');
    if (idx != std::string::npos) {
        return filename.substr(idx+1);
    }
    return "";
}

inline void printMultiline(const std::string &text,
                           size_t maxWidth,
                           size_t indentation=0,
                           const std::string &firstLine="") {
    std::string buffer;
    std::stringstream ss(text);

    std::vector<std::string> tokens;
    while (ss >> buffer) {
        tokens.push_back(buffer);
    }

    size_t currentWidth = indentation;
    std::cout << firstLine << "";
    std::cout << std::string(int(indentation) - int(firstLine.size()), ' ');
    for (size_t i = 0; i < tokens.size(); ++i) {
        size_t diff = tokens[i].size() + 1;

        if (currentWidth + diff > maxWidth) {
            std::cout << std::endl;
            std::cout << std::string(indentation, ' ');
            currentWidth = indentation;
        }
        std::cout << tokens[i] << " ";
        currentWidth += diff;
    }
}

template <typename T>
inline T lerp(T t, T min, T max) {
    return min + t * (max - min);
}

template <typename T>
inline T inverseLerp(T v, T min, T max) {
    return (v - min) / (max - min);
}

template <typename T>
inline T smoothstep(T edge0, T edge1, T x) {
    // https://docs.gl/sl4/smoothstep
    T t = clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
    return t * t * (T(3) - T(2) * t);
}

template <typename T>
inline T step(T edge, T x) {
    // https://docs.gl/sl4/step
    return smoothstep(edge, edge, x);
}

}
