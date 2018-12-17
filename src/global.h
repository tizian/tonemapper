/*
    src/global.h -- Global definitions and includes

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#ifdef _MSC_VER
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <cassert>
#include <memory>
#include <cmath>

using std::cout;
using std::cerr;
using std::endl;

inline float lerp(float t, float min, float max) {
    return min + t * (max - min);
}

inline float inverseLerp(float v, float min, float max) {
    return (v - min) / (max - min);
}

inline float clamp(float v, float min, float max) {
    return std::min(max, std::max(min, v));
}