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
	if (v <= min) return min;
	if (v >= max) return max;
	return v;
}