#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include "matrix4x4.h"
#include "hit_record.h"

// Forward declaration to avoid circular dependency
class BoundingBox;

class hittable {
public:
    virtual ~hittable() = default;
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    // Transform the object using a 4x4 matrix
    virtual void transform(const Matrix4x4& transform) {
        throw std::runtime_error("Transform not supported for this object.");
    }

    // Retrieve the bounding box of the object
    virtual BoundingBox bounding_box() const = 0;
};

#endif