#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include <SDL2/SDL.h>
#include <iostream>

using color = vec3;

void write_color(Uint32* pixels, int x, int y, int image_width, int image_height, const color& pixel_color) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Translate the [0,1] component values to the byte range [0,255].
    int rbyte = static_cast<int>(255.999 * r);
    int gbyte = static_cast<int>(255.999 * g);
    int bbyte = static_cast<int>(255.999 * b);

    // Write out the pixel color components to the SDL texture buffer.
    pixels[(image_height - 1 - y) * image_width + x] = (rbyte << 16) | (gbyte << 8) | bbyte;
}

#endif