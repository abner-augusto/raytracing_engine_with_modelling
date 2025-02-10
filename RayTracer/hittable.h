#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include "matrix4x4.h"
#include "hit_record.h"
#include "csg_intesection.h"

// Forward declaration to avoid circular dependency
class BoundingBox;

class hittable {
public:
    virtual ~hittable() = default;

    // Pure virtual function: must be implemented by derived classes
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    // Default function for non csg objects
    virtual bool csg_intersect(const ray& r, interval ray_t,
        std::vector<CSGIntersection>& out_intersections) const
    {
        hit_record rec;
        if (hit(r, ray_t, rec)) {
            out_intersections.emplace_back(rec.t, true, this, rec.normal, rec.p);
            return true;
        }
        return false;
    }

    // Default implementation for get_type_name()
    virtual std::string get_type_name() const {
        return "Unnamed";
    }

    // Transform the object using a 4x4 matrix
    virtual void transform(const Matrix4x4& transform) {
        throw std::runtime_error("Transform not supported for this object.");
    }

    virtual bool is_point_inside(const point3& p) const {
        return false;  // Default implementation
    }

    // Retrieve the bounding box of the object
    virtual BoundingBox bounding_box() const = 0;

    //Test if primitive is inside the bounding box
    virtual char test_bb(const BoundingBox& bb) const {
            return 'w'; // Default implementation
        }
};

#endif