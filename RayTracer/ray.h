#ifndef RAY_H
#define RAY_H

#include "vec3.h"
#include "matrix4x4.h"

class ray {
public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

    const point3& origin() const { return orig; }
    const vec3& direction() const { return dir; }

    point3 at(double t) const {
        return orig + t * dir;
    }

    // Transform the ray using a transformation matrix
    ray transform(const Matrix4x4& matrix) const {
        // Transform the origin as a point (applies translation)
        point3 new_origin = matrix.transform_point(orig);

        // Transform the direction as a vector (ignores translation)
        vec3 new_direction = matrix.transform_vector(dir);

        return ray(new_origin, new_direction);
    }

private:
    point3 orig;
    vec3 dir;
};

#endif