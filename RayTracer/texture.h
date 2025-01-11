#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "stb_image.h"
#include <string>
#include <cmath>

// Base class for texture representation
class texture {
public:
    virtual ~texture() = default; // Virtual destructor for proper cleanup
    virtual color value(double u, double v) const = 0; // Pure virtual function to get the color value at (u, v)
};

// Checkerboard texture class derived from texture
class checker_texture : public texture {
public:
    color color1, color2; // Two colors for the checker pattern
    double scale;         // Scale of the pattern

    // Constructor to initialize the checker texture with two colors and a scale
    checker_texture(const color& c1, const color& c2, double s)
        : color1(c1), color2(c2), scale(s) {
    }

    // Override the value function to determine the color based on UV coordinates
    color value(double u, double v) const override {
        auto sines = std::sin(scale * u * M_PI) * std::sin(scale * v * M_PI);
        return (sines < 0) ? color1 : color2; // Return color1 or color2 based on the sine value
    }
};

// Image texture class derived from texture
class image_texture : public texture {
public:
    unsigned char* data; // Pointer to the image data
    int width, height, channels;

    // Constructor to load the texture from a file
    image_texture(const std::string& filename)
        : data(nullptr), width(0), height(0), channels(0) {
        // Load image data using stb_image library
        data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (!data) {
            throw std::runtime_error("Failed to load texture: " + filename);
        }
    }

    // Destructor to free the image data
    ~image_texture() {
        if (data) stbi_image_free(data); // Free the loaded image data
    }

    // Override the value function to get the color at (u, v) from the image
    color value(double u, double v) const override {
        if (!data) return color(0, 1, 1); // Return a default color if no data is loaded

        // Wrap UV coordinates (no scaling needed here anymore)
        u = u - floor(u); // Wrap u to the range [0, 1]
        v = v - floor(v); // Wrap v to the range [0, 1]

        // Flip V to match image data
        v = 1.0 - v; // Invert v coordinate

        // Map UV to pixel coordinates
        int i = static_cast<int>(u * width);
        int j = static_cast<int>(v * height);

        // Clamp pixel coordinates to ensure they are within bounds
        i = std::clamp(i, 0, width - 1);
        j = std::clamp(j, 0, height - 1);

        // Compute pixel index in the data array
        int pixel_index = (j * width + i) * channels; 

        // Extract RGB values from the image data
        double r = data[pixel_index] / 255.0; // Normalize red value
        double g = data[pixel_index + 1] / 255.0; // Normalize green value
        double b = data[pixel_index + 2] / 255.0; // Normalize blue value

        return color(r, g, b);
    }
};

#endif