#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <algorithm>
#include <random>

// C++ Std Usings

using std::make_shared;
using std::shared_ptr;

// Constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Common Headers

#include "color.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"
#include "matrix4x4.h"
#include "hittable.h"
#include "hittable_list.h"

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * M_PI / 180.0;
}

inline double radians_to_degrees(double radians) {
    return radians * 180.0 / M_PI;
}

inline double random_double(double min, double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

inline point3 random_position() {
    return point3(
        random_double(-2.0, 2.0),  // x
        -0.15,                     // y
        random_double(-3.5, -1.0)  // z
    );
}

#endif