#ifndef HIT_RECORD_H
#define HIT_RECORD_H

#include "vec3.h"
#include "ray.h"
#include <memory>
#include <vector>

class mat;
class hittable;

struct CSGIntersection {
    double t;
    bool is_entry;
    const hittable* obj;
    vec3 normal;
    point3 p;
    const mat* material;

    CSGIntersection(double _t, bool _is_entry, const hittable* _obj,
        const vec3& _normal, const point3& _p, const mat* _material)
        : t(_t), is_entry(_is_entry), obj(_obj), normal(_normal), p(_p), material(_material) {
    }
};

class hit_record {
public:
    point3 p = point3(0, 0, 0);
    vec3 normal = vec3(0, 0, 0);
    double t = 0.0;
    bool front_face = true;
    const mat* material = nullptr;
    double u = 0.0;
    double v = 0.0;

    enum class CSGType { NONE, UNION, INTERSECTION, DIFFERENCE };
    CSGType csg_op = CSGType::NONE;
    const hittable* hit_object = nullptr;

    // List of sub-intersections relevant to CSG computations
    std::vector<CSGIntersection> csg_intersections;

    hit_record() = default;

    void reset() {
        p = point3(0, 0, 0);
        normal = vec3(0, 0, 0);
        t = 0.0;
        front_face = true;
        material = nullptr;
        u = v = 0.0;
        csg_op = CSGType::NONE;
        hit_object = nullptr;
        csg_intersections.clear();
    }

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }

    bool is_csg_hit() const {
        return csg_op != CSGType::NONE && !csg_intersections.empty();
    }
};

#endif // HIT_RECORD_H