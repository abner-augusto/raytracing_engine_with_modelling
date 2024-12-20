#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "stb_image.h"
#include <string>
#include <cmath>

class texture {
public:
    virtual ~texture() = default;
    virtual color value(double u, double v) const = 0; // Retorna cor com base em UV
};

class checker_texture : public texture {
public:
    color color1, color2; // Duas cores do padrão xadrez
    double scale;         // Escala do padrão

    checker_texture(const color& c1, const color& c2, double s)
        : color1(c1), color2(c2), scale(s) {
    }

    color value(double u, double v) const override {
        auto sines = std::sin(scale * u * M_PI) * std::sin(scale * v * M_PI);
        return (sines < 0) ? color1 : color2;
    }
};

class image_texture : public texture {
public:
    unsigned char* data;
    int width, height, channels;
    double scale_u, scale_v; // Scaling factors for U and V

    // Constructor with optional scaling factors
    image_texture(const std::string& filename, double scale_u = 1.0, double scale_v = 1.0)
        : scale_u(scale_u), scale_v(scale_v) {
        data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (!data) {
            throw std::runtime_error("Failed to load texture: " + filename);
        }
    }

    ~image_texture() {
        stbi_image_free(data);
    }

    virtual color value(double u, double v) const override {
        if (!data) return color(0, 1, 1); // Debug color (magenta) for missing textures

        // Apply scaling factors
        u *= scale_u;
        v *= scale_v;

        // Clamp UV coordinates to [0, 1]
        u = u - floor(u); // Wrap around (repeating texture)
        v = v - floor(v); // Wrap around (repeating texture)

        // Flip V to match image data
        v = 1.0 - v;

        // Map UV to pixel coordinates
        int i = static_cast<int>(u * width);
        int j = static_cast<int>(v * height);

        // Clamp pixel coordinates
        i = i < 0 ? 0 : (i >= width ? width - 1 : i);
        j = j < 0 ? 0 : (j >= height ? height - 1 : j);

        // Compute pixel index
        int pixel_index = (j * width + i) * channels;

        // Extract RGB values
        double r = data[pixel_index] / 255.0;
        double g = data[pixel_index + 1] / 255.0;
        double b = data[pixel_index + 2] / 255.0;

        return color(r, g, b);
    }
};

#endif
