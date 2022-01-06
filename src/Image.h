/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <Global.h>

#include <Color.h>

namespace tonemapper {

class Image {
public:
    Image(size_t width, size_t height);
    ~Image();

    void precompute();

    static Image *load(const std::string &filename);
    void save(const std::string &filename) const;

    float *getData() { return (float *) m_pixels.get(); }

    const Color3f &ref(size_t i, size_t j) const;
    Color3f &ref(size_t i, size_t j);

    inline size_t getWidth() const { return m_width; }
    inline size_t getHeight() const { return m_height; }

    inline Color3f getMean()    const { return m_mean; }
    inline Color3f getMaximum() const { return m_max; }
    inline float getMinimumLuminance() const { return m_minimumLuminance; }
    inline float getMaximumLuminance() const { return m_maximumLuminance; }
    inline float getMeanLuminance() const { return m_meanLuminance; }
    inline float getLogMeanLuminance() const { return m_logMeanLuminance; }

    inline const std::string &getFilename() const { return m_filename; };
    inline void setFilename(const std::string &filename) { m_filename = filename; }

private:
    // Image data
    size_t m_width, m_height;
    std::unique_ptr<Color3f[]> m_pixels;
    std::string m_filename;

    // Precomputed values used by some operators
    Color3f m_mean,
            m_max;
    float m_minimumLuminance,
          m_maximumLuminance,
          m_meanLuminance,
          m_logMeanLuminance;
};

} // Namespace tonemapper
