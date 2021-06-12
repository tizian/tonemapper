#pragma once

#define YEAR    "2021"
#define VERSION "2.0.dev"

#include <tinyformat.h>

#define TERM_COLOR_RED    "\x1B[31m"
#define TERM_COLOR_YELLOW "\x1B[33m"
#define TERM_COLOR_WHITE  "\x1B[37m"

#define LOG(str, ...)   std::cout << tfm::format(str "\n", ##__VA_ARGS__)
#define PRINT(str, ...) std::cout << tfm::format(str "\n", ##__VA_ARGS__)
#define INFO(str, ...)  std::cout << tfm::format("%s(%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define WARN(str, ...)  std::cout << tfm::format(TERM_COLOR_YELLOW "%s(%d): " str "\n" TERM_COLOR_WHITE, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERROR(str, ...) throw std::runtime_error(tfm::format(TERM_COLOR_RED "\nError - %s(%d): " str "\n" TERM_COLOR_WHITE, __FILE__, __LINE__, ##__VA_ARGS__))

#define VARLOG(x)           (std::cout << #x << ": " << (x) << std::endl)
#define VARLOG2(x, y)       (std::cout << #x << ": " << (x) << ", " << #y << ": " << (y) << std::endl)
#define VARLOG3(x, y, z)    (std::cout << #x << ": " << (x) << ", " << #y << ": " << (y) << ", " << #z << ": " << (z) << std::endl)
#define VARLOG4(x, y, z, w) (std::cout << #x << ": " << (x) << ", " << #y << ": " << (y) << ", " << #z << ": " << (z) << ", " << #w << ": " << (w) << std::endl)
