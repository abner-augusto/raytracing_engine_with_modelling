#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
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

#endif
