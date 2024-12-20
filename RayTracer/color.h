#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include "interval.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <array>  

using color = vec3;

inline void write_color(Uint32* pixels, int x, int y, int image_width, int image_height, const color& pixel_color) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Translate the [0,1] component values to the byte range [0,255].
    static const interval intensity(0.000, 0.999);
    int rbyte = int(256 * intensity.clamp(r));
    int gbyte = int(256 * intensity.clamp(g));
    int bbyte = int(256 * intensity.clamp(b));

    // Write out the pixel color components to the SDL texture buffer.
    pixels[(image_height - 1 - y) * image_width + x] = (rbyte << 16) | (gbyte << 8) | bbyte;
}

inline color clamp(const color& c, double minVal, double maxVal) {
    return color(
        std::max(minVal, std::min(c.x(), maxVal)),
        std::max(minVal, std::min(c.y(), maxVal)),
        std::max(minVal, std::min(c.z(), maxVal))
    );
}

// Função auxiliar para gerar uma cor aleatória
inline vec3 random_color() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);

    return vec3(dis(gen), dis(gen), dis(gen));
}

#endif