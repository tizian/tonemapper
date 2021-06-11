#pragma once

#include <Global.h>

#include <Color.h>

namespace tonemapper {

class Image {
public:
    Image(size_t width, size_t height);
    ~Image();

    static Image *load(const std::string &filename);
    void save(const std::string &filename) const;

    float *getData() { return (float *) m_pixels.get(); }

    const Color3f &ref(int i, int j) const;
    Color3f &ref(int i, int j);

    inline int getWidth() const { return m_width; }
    inline int getHeight() const { return m_height; }

    inline Color3f getMean() const { return m_mean; }
    inline float getMinimumLuminance() const { return m_minimumLuminance; }
    inline float getMaximumLuminance() const { return m_maximumLuminance; }
    inline float getMeanLuminance() const { return m_meanLuminance; }
    inline float getLogMeanLuminance() const { return m_logMeanLuminance; }
    inline float getAutoKeyValue() const { return m_autoKeyValue; }

    inline const std::string &getFilename() const { return m_filename; };
    inline void setFilename(const std::string &filename) { m_filename = filename; }

private:
    // Image data
    size_t m_width, m_height;
    std::unique_ptr<Color3f[]> m_pixels;
    std::string m_filename;

    // Precomputed values used by some operators
    Color3f m_mean;
    float m_minimumLuminance,
          m_maximumLuminance,
          m_meanLuminance,
          m_logMeanLuminance,
          m_autoKeyValue;
};

} // Namespace tonemapper
