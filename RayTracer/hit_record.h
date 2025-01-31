#ifndef HIT_RECORD_H
#define HIT_RECORD_H

#include "vec3.h"
#include "ray.h"
#include <memory>
#include <vector>

class mat;
class hittable;

class hit_record {
public:
    point3 p = point3(0, 0, 0);
    vec3 normal = vec3(0, 0, 0);
    double t = 0.0;
    bool front_face = true;
    const mat* material = nullptr;
    double u = 0.0;
    double v = 0.0;

    const hittable* hit_object = nullptr;


    hit_record() = default;

    void reset() {
        p = point3(0, 0, 0);
        normal = vec3(0, 0, 0);
        t = 0.0;
        front_face = true;
        material = nullptr;
        u = v = 0.0;
        hit_object = nullptr;
    }

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }

};

#endif // HIT_RECORD_H