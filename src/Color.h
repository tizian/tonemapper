/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <cmath>

namespace tonemapper {

struct Color3f {
    Color3f(float v = 0.f) {
        c[0] = c[1] = c[2] = v;
    }

    Color3f(float r, float g, float b) {
        c[0] = r;
        c[1] = g;
        c[2] = b;
    }

    Color3f &operator+=(const Color3f &c2) {
        c[0] += c2[0];
        c[1] += c2[1];
        c[2] += c2[2];
        return *this;
    }

    Color3f operator+(const Color3f &c2) const {
        Color3f ret = *this;
        ret[0] += c2[0];
        ret[1] += c2[1];
        ret[2] += c2[2];
        return ret;
    }

    Color3f operator-() const {
        Color3f ret;
        ret[0] = -c[0];
        ret[1] = -c[1];
        ret[2] = -c[2];
        return ret;
    }

    Color3f &operator-=(const Color3f &c2) {
        c[0] -= c2[0];
        c[1] -= c2[1];
        c[2] -= c2[2];
        return *this;
    }

    Color3f operator-(const Color3f &c2) const {
        Color3f ret = *this;
        ret[0] -= c2[0];
        ret[1] -= c2[1];
        ret[2] -= c2[2];
        return ret;
    }

    Color3f &operator/=(const Color3f &c2) {
        c[0] /= c2[0];
        c[1] /= c2[1];
        c[2] /= c2[2];
        return *this;
    }

    Color3f operator/(const Color3f &c2) const {
        Color3f ret = *this;
        ret[0] /= c2[0];
        ret[1] /= c2[1];
        ret[2] /= c2[2];
        return ret;
    }

    Color3f &operator/=(float s) {
        c[0] /= s;
        c[1] /= s;
        c[2] /= s;
        return *this;
    }

    Color3f operator/(float s) const {
        Color3f ret = *this;
        ret[0] /= s;
        ret[1] /= s;
        ret[2] /= s;
        return ret;
    }

    Color3f &operator*=(const Color3f &c2) {
        c[0] *= c2[0];
        c[1] *= c2[1];
        c[2] *= c2[2];
        return *this;
    }

    Color3f operator*(const Color3f &c2) const {
        Color3f ret = *this;
        ret[0] *= c2[0];
        ret[1] *= c2[1];
        ret[2] *= c2[2];
        return ret;
    }

    Color3f &operator*=(float s) {
        c[0] *= s;
        c[1] *= s;
        c[2] *= s;
        return *this;
    }

    Color3f operator*(float s) const {
        Color3f ret = *this;
        ret[0] *= s;
        ret[1] *= s;
        ret[2] *= s;
        return ret;
    }

    friend inline Color3f operator*(float s, const Color3f &c) {
        return c * s;
    }

    float &operator[](int i) {
        return c[i];
    }

    float operator[](int i) const {
        return c[i];
    }

    float& operator[](size_t i) {
        return c[i];
    }

    float operator[](size_t i) const {
        return c[i];
    }

    bool operator==(const Color3f &c2) const {
        for (int i = 0; i < 3; ++i) {
            if (c[i] != c2[i]) return false;
        }
        return true;
    }

    bool operator!=(const Color3f &c2) const {
        return !(*this == c2);
    }

    friend float mean(const Color3f &c) {
        float sum = c[0] + c[1] + c[2];
        return sum / 3.f;
    }

    friend Color3f sqrt(const Color3f &c) {
        Color3f ret;
        ret[0] = std::sqrt(c[0]);
        ret[1] = std::sqrt(c[1]);
        ret[2] = std::sqrt(c[2]);
        return ret;
    }

    friend Color3f sqr(const Color3f &c) {
        Color3f ret;
        ret[0] = c[0]*c[0];
        ret[1] = c[1]*c[1];
        ret[2] = c[2]*c[2];
        return ret;
    }

    friend Color3f exp(const Color3f &c) {
        Color3f ret;
        ret[0] = std::exp(c[0]);
        ret[1] = std::exp(c[1]);
        ret[2] = std::exp(c[2]);
        return ret;
    }

    friend Color3f pow(const Color3f &c, float exponent) {
        Color3f ret;
        ret[0] = std::pow(c[0], exponent);
        ret[1] = std::pow(c[1], exponent);
        ret[2] = std::pow(c[2], exponent);
        return ret;
    }

    friend Color3f clamp(const Color3f &c, float low, float high) {
        Color3f ret;
        ret[0] = std::max(std::min(c[0], high), low);
        ret[1] = std::max(std::min(c[1], high), low);
        ret[2] = std::max(std::min(c[2], high), low);
        return ret;
    }

    friend Color3f clamp(const Color3f &c, Color3f low, Color3f high) {
        Color3f ret;
        ret[0] = std::max(std::min(c[0], high[0]), low[0]);
        ret[1] = std::max(std::min(c[1], high[1]), low[1]);
        ret[2] = std::max(std::min(c[2], high[2]), low[2]);
        return ret;
    }

    friend Color3f clampPositive(const Color3f &c) {
        Color3f ret;
        ret[0] = std::max(c[0], 0.f);
        ret[1] = std::max(c[1], 0.f);
        ret[2] = std::max(c[2], 0.f);
        return ret;
    }

    friend float min(const Color3f &c) {
        float ret = std::numeric_limits<float>::infinity();
        for (int i = 0; i < 3; ++i) {
            if (c[i] < ret) ret = c[i];
        }
        return ret;
    }

    friend float max(const Color3f &c) {
        float ret = -std::numeric_limits<float>::infinity();
        for (int i = 0; i < 3; ++i) {
            if (c[i] > ret) ret = c[i];
        }
        return ret;
    }

    friend Color3f min(const Color3f &c1, const Color3f &c2) {
        Color3f ret;
        ret[0] = std::min(c1[0], c2[0]);
        ret[1] = std::min(c1[1], c2[1]);
        ret[2] = std::min(c1[2], c2[2]);
        return ret;
    }

    friend Color3f max(const Color3f &c1, const Color3f &c2) {
        Color3f ret;
        ret[0] = std::max(c1[0], c2[0]);
        ret[1] = std::max(c1[1], c2[1]);
        ret[2] = std::max(c1[2], c2[2]);
        return ret;
    }

    bool isBlack() const {
        for (int i = 0; i < 3; ++i) {
            if (c[i] != 0.f) return false;
        }
        return true;
    }

    bool isValid() const {
        for (int i = 0; i < 3; ++i) {
            if (c[i] < 0.f || !std::isfinite(c[i])) return false;
        }
        return true;
    }

    float &r() { return c[0]; }
    const float &r() const { return c[0]; }

    float &g() { return c[1]; }
    const float &g() const { return c[1]; }

    float &b() { return c[2]; }
    const float &b() const { return c[2]; }

    friend float luminance(const Color3f &c) {
        return c[0] * 0.212671f + c[1] * 0.715160f + c[2] * 0.072169f;
    }

    friend float luminanceRods(const Color3f &c) {
        /* From "A Multiscale Model of Adaptation and Spatial Vision for
           Realistic Image Display" by Pattanaik et al. 1998 */
        float X = 0.412453f * c.r() + 0.357580f * c.g() + 0.180423f * c.b(),
              Y = 0.212671f * c.r() + 0.715160f * c.g() + 0.072169f * c.b(),
              Z = 0.019334f * c.r() + 0.119193f * c.g() + 0.950227f * c.b();
        return -0.702f * X + 1.039f * Y + 0.433f * Z;
    }

    friend std::ostream &operator<<(std::ostream &os, const Color3f &c) {
        os << "[" << c[0] << ", " << c[1] << ", " << c[2] << "]";
        return os;
    }

private:
    float c[3];
};



} // Namespace tonemapper
