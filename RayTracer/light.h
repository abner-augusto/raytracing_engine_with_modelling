#ifndef LIGHT_H
#define LIGHT_H

#include "color.h"

class Light {
public:
    vec3 position;
    double intensity;
    color light_color;

    Light(const vec3& pos, double inten, const color& col)
        : position(pos), intensity(inten), light_color(col) {
    }

    ~Light() = default;


};

#endif
