#ifndef TEXTURE_H
#define TEXTURE_H
#include "color.h"
#include "stb_image.h"
#include <string>
#include <cmath>
#include <stdexcept>

// Base texture class
class texture {
public:
    virtual ~texture() = default;
    virtual color value(double u, double v) const = 0;
    // New method to check if texture is valid
    virtual bool is_valid() const = 0;
};

// Checker pattern texture
class checker_texture : public texture {
public:
    color color1, color2;
    double scale; // Controls the frequency of the sine wave pattern

    checker_texture(const color& c1, const color& c2, double s)
        : color1(c1), color2(c2), scale(s) {
    }

    virtual color value(double u, double v) const override {
        // Apply sine wave to the UV coordinates
        double sine_pattern = sin(u * scale * M_PI) * sin(v * scale * M_PI);

        // Use the sign of the sine pattern to alternate colors
        if (sine_pattern > 0) {
            return color1;
        }
        else {
            return color2;
        }
    }

    virtual bool is_valid() const override {
        return true; // Checker textures are always valid
    }
};

// Image-based texture
class image_texture : public texture {
private:
    unsigned char* data;
    int width, height, channels;
    bool valid;

public:
    image_texture(const std::string& filename)
        : data(nullptr)
        , width(0)
        , height(0)
        , channels(0)
        , valid(false) {
        try {
            data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
            valid = (data != nullptr);
        }
        catch (...) {
            valid = false;
        }
    }

    ~image_texture() {
        if (data) {
            stbi_image_free(data);
        }
    }

    bool is_valid() const override { return valid && data != nullptr; }

    color value(double u, double v) const override {
        if (!is_valid()) {
            throw std::runtime_error("Attempting to sample invalid texture");
        }

        // Wrap UV coordinates
        u = u - std::floor(u);
        v = 1.0 - (v - std::floor(v)); // Flip V coordinate

        // Map to pixel coordinates
        int i = static_cast<int>(u * width);
        int j = static_cast<int>(v * height);

        // Clamp coordinates
        i = std::clamp(i, 0, width - 1);
        j = std::clamp(j, 0, height - 1);

        // Get pixel data
        int pixel_index = (j * width + i) * channels;

        // Convert to normalized RGB
        double r = data[pixel_index] / 255.0;
        double g = data[pixel_index + 1] / 255.0;
        double b = data[pixel_index + 2] / 255.0;

        return color(r, g, b);
    }
};

#endif