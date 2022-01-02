#include <Image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// #define STB_IMAGE_IMPLEMENTATION // TODO: nanogui?
#include <stb_image.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#include <limits>

namespace tonemapper {

Image::Image(size_t width, size_t height)
    : m_width(width), m_height(height) {
    m_pixels = std::unique_ptr<Color3f[]>(new Color3f[m_width * m_height]);
}

Image::~Image() {}

Image *loadFromEXR(const std::string &filename) {
    const char *filename_c = filename.c_str();
    const char *err = nullptr;

    EXRVersion version;
    EXRHeader header;
    InitEXRHeader(&header);

    if (ParseEXRVersionFromFile(&version, filename_c) != 0 ||
        ParseEXRHeaderFromFile(&header, &version, filename_c, &err) != 0) {
        ERROR("Bitmap(): Could not parse EXR file \"%s\". %s", filename, err);
        // FreeEXRErrorMessage(err);
    }

    for (int i = 0; i < header.num_channels; ++i) {
        if (header.requested_pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
            header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        }
    }

    EXRImage img;
    InitEXRImage(&img);
    if (LoadEXRImageFromFile(&img, &header, filename_c, &err) != 0) {
        ERROR("Bitmap(): Could not open EXR file \"%s\". %s", filename, err);
        // FreeEXRErrorMessage(err);
    }

    Image *result = new Image(img.width, img.height);

    int channels = img.num_channels;
    int *chIdx = new int[channels];
    for (int ch = 0; ch < channels; ++ch) {
        if (strcmp(header.channels[ch].name, "R") == 0) chIdx[0] = ch;
        if (strcmp(header.channels[ch].name, "G") == 0) chIdx[1] = ch;
        if (strcmp(header.channels[ch].name, "B") == 0) chIdx[2] = ch;
        if (strcmp(header.channels[ch].name, "A") == 0) chIdx[3] = ch;
    }
    channels = std::min(3, channels);    // At most read in 3 channels, without alpha

    auto convert = [](void *p, int offset, int type) {
        switch(type) {
            case TINYEXR_PIXELTYPE_UINT: {
                int32_t *pix = (int32_t *)p;
                return (float)pix[offset];
            }
            case TINYEXR_PIXELTYPE_HALF:
            case TINYEXR_PIXELTYPE_FLOAT: {
                float *pix = (float *)p;
                return pix[offset];
            }
            default:
                return 0.f;
        }
    };

    int offset = 0;
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j, ++offset) {
            Color3f c;
            if (channels == 3) {
                for (int ch = 0; ch < channels; ++ch) {
                    int ch_ = chIdx[ch];
                    c[ch] = convert(img.images[ch_], offset, header.pixel_types[ch_]);
                }
            } else {
                int ch_ = chIdx[0];
                c = convert(img.images[ch_], offset, header.pixel_types[ch_]);
            }
            result->ref(i, j) = c;
        }
    }

    FreeEXRImage(&img);
    FreeEXRHeader(&header);
    delete[] chIdx;

    return result;
}

Image *loadFromHDR(const std::string &filename) {
    int width, height, channels;
    float *data = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);

    Image *result = new Image(width, height);

    // At most read in 3 channels, without alpha
    channels = std::min(3, channels);

    float *src = data;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            Color3f c;
            if (channels == 3) {
                for (int ch = 0; ch < 3; ++ch) {
                    c[ch] = src[0];
                    src++;
                }
            } else {
                c = src[0];
                src++;
            }
            result->ref(i, j) = c;
        }
    }

    stbi_image_free(data);
    return result;
}

Image *Image::load(const std::string &filename) {
    Image *image = nullptr;

    std::string extension = fileExtension(filename);
    if (extension == "exr") {
        image = loadFromEXR(filename);
    } else if (extension == "hdr") {
        image = loadFromHDR(filename);
    } else if (extension == "") {
        ERROR("Image::load(): Did not recognize file extension for \"%s\".", filename);
    } else {
        ERROR("Image::load(): Invalid file extension in \"%s\". Only \".exr\" or \".hdr\" formats are supported.", filename);
    }

    if (image) {
        image->setFilename(filename);
        image->precompute();
        return image;
    }

    ERROR("Image::load(): Something went wrong when loading \"%s\".", filename);
    return nullptr;
}

void Image::save(const std::string &filename) const {
    std::string out = filename;
    bool saveAsJpg;

    std::string extension = fileExtension(filename);
    if (extension == "jpg") {
        saveAsJpg = true;
    } else if (extension == "png") {
        saveAsJpg = false;
    } else if (extension == "") {
        // No extension provided, automatically save as .jpg
        saveAsJpg = true;
        out += ".jpg";
    } else {
        ERROR("Image::save(): Invalid file extension in \"%s\". Can only save as either \".png\" or \".jpg\" format.", filename);
    }

    uint8_t *rgb8 = new uint8_t[3 * m_width * m_height];
    uint8_t *dst = rgb8;

    float *data = (float *) m_pixels.get();
    for (size_t i = 0; i < m_height; ++i) {
        for (size_t j = 0; j < m_width; ++j) {
            size_t idx = i*m_width + j;
            for (size_t ch = 0; ch < 3; ++ch) {

                /* At this point, any tonemapping operator
                   should already be applied, so we just
                   save the raw data. */
                float v = data[3*idx + ch];
                v = std::min(1.f, std::max(0.f, v));
                dst[0] = uint8_t(255.f * v);
                dst++;
            }
        }
    }

    int ret;

    if (saveAsJpg) {
        ret = stbi_write_jpg(out.c_str(), m_width, m_height, 3, rgb8, 100);
    } else {
        ret = stbi_write_png(out.c_str(), m_width, m_height, 3, rgb8, 3 * m_width);
    }

    if (ret == 0) {
        ERROR("save(): Could not save file \"%s\"", out);
    }

    delete[] rgb8;
}

const Color3f &Image::ref(size_t i, size_t j) const {
    return m_pixels[m_width * i + j];
}

Color3f &Image::ref(size_t i, size_t j) {
    return m_pixels[m_width * i + j];
}

void Image::precompute() {
    m_minimumLuminance =  std::numeric_limits<float>::infinity();
    m_maximumLuminance = -std::numeric_limits<float>::infinity();
    m_meanLuminance = 0.f;
    m_logMeanLuminance = 0.f;
    m_mean = Color3f(0.f);
    m_max  = Color3f(-std::numeric_limits<float>::infinity());

    size_t N = 0;
    for (size_t i = 0; i < m_height; ++i) {
        for (size_t j = 0; j < m_width; ++j) {
            const Color3f &color = ref(i, j);
            m_mean += color;
            m_max = max(m_max, color);

            float L = luminance(color);
            m_minimumLuminance = std::min(m_minimumLuminance, L);
            m_maximumLuminance = std::max(m_maximumLuminance, L);
            m_meanLuminance += L;

            if (L > 0.f) {
                /* Be careful here as the log is only defined for non-zero
                   luminance values.
                   "Image Processing Techniques" by McReynolds et al. 2005
                   suggest to alternatively add a small `delta` biasing term to
                   avoid log(0), but this is not sufficient in case the image
                   contains many black pixels. */
                m_logMeanLuminance += std::log(L);
                N++;
            }
        }
    }
    m_mean /= (m_height * m_width);
    m_meanLuminance /= (m_height * m_width);

    /* Eq. (1) in Eq. (1) in "Photographic Tone Reproduction for Digital Images"
       by Reinhard et al. 2002. divides by N after exponentiating. But this does not
       give sensible values here. Instead, the whole expression should be equivalent
       to computing a geometric mean. */
    m_logMeanLuminance = std::exp(m_logMeanLuminance / N);
}

} // Namespace tonemapper
