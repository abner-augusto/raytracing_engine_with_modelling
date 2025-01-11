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

    // Transform method to apply a Matrix4x4 transformation
    void transform(const Matrix4x4& matrix) {
        //std::cout << "Applying transformation to light at: " << position << "\n";
        position = matrix.transform_point(position);
        //std::cout << "New position: " << position << "\n";
    }

};


#endif
