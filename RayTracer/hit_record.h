#ifndef HIT_RECORD_H
#define HIT_RECORD_H

#include "vec3.h"
#include "ray.h"
#include <memory>

class mat; // Forward declaration for material
class hittable; // Forward declaration to avoid compilation error

class hit_record {
public:
    point3 p = point3(0, 0, 0);    // Collision point
    vec3 normal = vec3(0, 0, 0);   // Normal at collision point
    double t = 0.0;                // Ray parameter
    bool front_face = true;        // Indicates if the collision is external or internal
    const mat* material = nullptr; // Pointer to the material
    double u = 0.0;                // Horizontal UV coordinate
    double v = 0.0;                // Vertical UV coordinate

    // Additional fields for CSG
    enum class CSGType { NONE, UNION, INTERSECTION, DIFFERENCE };
    CSGType csg_op = CSGType::NONE;  // Tracks which operation was used in the CSG tree
    bool inside = false;       
    bool is_entry = true;
    const hittable* hit_object = nullptr;  // Pointer to the hit object

    hit_record() = default;

    void reset() {
        p = point3(0, 0, 0);
        normal = vec3(0, 0, 0);
        t = 0.0;
        front_face = true;
        material = nullptr;
        u = 0.0;
        v = 0.0;
        csg_op = CSGType::NONE;
        hit_object = nullptr;
    }

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }

    // Helper to check if it's a CSG hit
    bool is_csg_hit() const {
        return csg_op != CSGType::NONE;
    }
};

#endif
