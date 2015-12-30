/*
    src/image.cpp -- Image datatype
    
    Copyright (c) 2015 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license. 
*/

#include <image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#include <iostream>

float convert(void *p, int offset, int type) {
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
}

const Color3f &Image::ref(int i, int j) const {
	return m_pixels[m_size.x() * i + j];
}

Color3f &Image::ref(int i, int j) {
	return m_pixels[m_size.x() * i + j];
}

Image::Image(const std::string &filename) {
	EXRImage img;
	InitEXRImage(&img);

	const char *err = nullptr;
	if (ParseMultiChannelEXRHeaderFromFile(&img, filename.c_str(), &err) != 0) {
		std::cerr << "Error: Could not parse EXR file: " << err << std::endl;
		return;
	}

	for (int i = 0; i < img.num_channels; ++i) {
        if (img.requested_pixel_types[i] == TINYEXR_PIXELTYPE_HALF)
            img.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    }

    if (LoadMultiChannelEXRFromFile(&img, filename.c_str(), &err) != 0) {
        std::cerr << "Error: Could not open EXR file: " << err << std::endl;
        return;
    }

	m_size = Eigen::Vector2i(img.width, img.height);

	m_pixels = std::unique_ptr<Color3f[]>(new Color3f[m_size.x() * m_size.y()]);

	int idxR = -1, idxG = -1, idxB = -1;
	for (int c = 0; c < img.num_channels; ++c) {
		if (strcmp(img.channel_names[c], "R") == 0) {
			idxR = c;
		}
		if (strcmp(img.channel_names[c], "G") == 0) {
			idxG = c;
		}
		if (strcmp(img.channel_names[c], "B") == 0) {
			idxB = c;
		}
	}

	float rgb[3];
	for (int i = 0; i < m_size.y(); ++i) {
		for (int j = 0; j < m_size.x(); ++j) {
			int index = m_size.x() * i + j;

			if (img.num_channels == 1) {
				rgb[0] = convert(img.images[0], index, img.pixel_types[0]);
				ref(i, j) = Color3f(rgb[0]);
			}
			else {
				rgb[0] = convert(img.images[idxR], index, img.pixel_types[idxR]);
				rgb[1] = convert(img.images[idxG], index, img.pixel_types[idxG]);
				rgb[2] = convert(img.images[idxB], index, img.pixel_types[idxB]);
				ref(i, j) = Color3f(rgb[0], rgb[1], rgb[2]);
			}
		}
	}

	float delta = 1e-4f;
	m_averageLuminance = 0.f;
	m_logAverageLuminance = 0.f;
	m_minimumLuminance = std::numeric_limits<float>::max();
	m_maximumLuminance = std::numeric_limits<float>::min();
	m_averageIntensity = Color3f(0.f);

	for (int i = 0; i < m_size.y(); ++i) {
		for (int j = 0; j < m_size.x(); ++j) {
			m_averageIntensity += ref(i, j);
			float lum = ref(i, j).getLuminance();
			m_averageLuminance += lum;
			m_logAverageLuminance += std::log(delta + lum);
			if (lum > m_maximumLuminance) m_maximumLuminance = lum;
			if (lum < m_minimumLuminance) m_minimumLuminance = lum;
		}
	}

	m_averageIntensity = m_averageIntensity / (m_size.x() * m_size.y());
	m_averageLuminance = m_averageLuminance / (m_size.x() * m_size.y());
	m_logAverageLuminance = std::exp(m_logAverageLuminance / (m_size.x() * m_size.y()));

	// Formula taken from "Perceptual Effects in Real-time Tone Mapping" by Krawczyk et al.
	m_autoKeyValue = 1.03f - 2.f / (2.f + std::log10(m_logAverageLuminance + 1.f));

	FreeEXRImage(&img);
}

inline uint8_t convert(float v) {
	return (uint8_t) clamp(255.f * v, 0.f, 255.f);
}

void Image::saveAsPNG(const std::string &filename, TonemapOperator *tonemap, float exposure, float *progress) const {
	uint8_t *rgb8 = new uint8_t[3 * m_size.x() * m_size.y()];
	uint8_t *dst = rgb8;

	tonemap->process(this, dst, exposure, progress);

	int ret = stbi_write_png(filename.c_str(), m_size.x(), m_size.y(), 3, rgb8, 3 * m_size.x());
	if (ret == 0) {
		cerr << "Error: Could not save PNG file" << endl;
	}

	*progress = -1.f;

	delete[] rgb8;
}