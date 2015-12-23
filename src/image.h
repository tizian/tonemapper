#pragma once

#include <global.h>

#include <color.h>
#include <tonemap.h>

class Image {
public:
    explicit Image(const std::string &filename);
    ~Image() {}

    float *getData() { return (float *)m_pixels.get(); }

    const Color3f &ref(int i, int j) const;
    Color3f &ref(int i, int j);

    inline float getMinimumLuminance() const { return m_minimumLuminance; }
    inline float getMaximumLuminance() const { return m_maximumLuminance; }
    inline float getAverageLuminance() const { return m_averageLuminance; }
    inline float getLogAverageLuminance() const { return m_logAverageLuminance; }
	inline float getAutoKeyValue() const { return m_autoKeyValue; }

    inline const Eigen::Vector2i &getSize() const { return m_size; }
    inline int getWidth() const { return m_size.x(); }
    inline int getHeight() const { return m_size.y(); }

    void saveAsPNG(const std::string &filename, TonemapOperator *tonemap, float exposure = 1.f) const;

private:
    std::unique_ptr<Color3f[]> m_pixels;

    Eigen::Vector2i m_size;

    float m_minimumLuminance;
    float m_maximumLuminance;
    float m_averageLuminance;
    float m_logAverageLuminance;
	float m_autoKeyValue;
};