#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include <cmath>

class triangle : public hittable {
public:
    triangle(const point3& v0, const point3& v1, const point3& v2, const mat& material)
        : v0(v0), v1(v1), v2(v2), material(material) {
        update_triangle();
    }

    void update_triangle(const point3& new_v0, const point3& new_v1, const point3& new_v2) {
        v0 = new_v0;
        v1 = new_v1;
        v2 = new_v2;
        update_triangle();
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const vec3 dir = r.direction();
        const vec3 orig = r.origin();

        vec3 rov0 = orig - v0;
        double d = dot(dir, n);

        if (std::fabs(d) < 1e-8)
            return false;

        vec3 q = cross(rov0, dir);
        double inv_d = 1.0 / d;
        double u = inv_d * dot(-q, v2v0);
        double v = inv_d * dot(q, v1v0);
        double t = inv_d * dot(-n, rov0);

        if (u < 0.0 || v < 0.0 || (u + v) > 1.0 || !ray_t.contains(t))
            return false;

        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, normal);
        rec.material = &material;
        return true;
    }

private:
    point3 v0, v1, v2;
    vec3 v1v0, v2v0;
    vec3 n;       // normal não normalizada
    vec3 normal;  // normal unitária
    const mat& material;

    // Função interna para recalcular vetores e normal do triângulo
    void update_triangle() {
        v1v0 = v1 - v0;
        v2v0 = v2 - v0;
        n = cross(v1v0, v2v0);
        normal = unit_vector(n);
    }
};

#endif
