#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include <algorithm>
#include <cmath>

class triangle : public hittable {
public:
    triangle(const point3& _v0, const point3& _v1, const point3& _v2, const mat& m)
        : v0(_v0), v1(_v1), v2(_v2), material(m)
    {
        update_triangle();
    }

    // Update the vertices and recalculate precomputed values
    void update_triangle(const point3& _v0, const point3& _v1, const point3& _v2) {
        v0 = _v0;
        v1 = _v1;
        v2 = _v2;
        update_triangle();
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

        double dot_dir_normal = dot(r.direction(), normal);
        if (dot_dir_normal > 0.0) {
            return false;
        }

        // E1 and E2 are the two edges from v0.
        const vec3 E1 = edge01;
        const vec3 E2 = edge02;

        // Step 1: Calculate P = cross(ray direction, E2)
        const vec3 P = cross(r.direction(), E2);
        const double determinant = dot(E1, P);

        // Step 2: If determinant is near 0, the ray is almost parallel
        if (std::fabs(determinant) < 1e-8) {
            return false;
        }
        const double invDet = 1.0 / determinant;

        // Step 3: Calculate T = (ray origin - v0), and compute u
        const vec3 T = r.origin() - v0;
        const double u = dot(T, P) * invDet;
        if (u < 0.0 || u > 1.0) {
            return false;
        }

        // Step 4: Q = cross(T, E1), then compute v
        const vec3 Q = cross(T, E1);
        const double v = dot(r.direction(), Q) * invDet;
        if (v < 0.0 || (u + v) > 1.0) {
            return false;
        }

        // Step 5: intersection distance t
        const double t = dot(E2, Q) * invDet;
        if (!ray_t.contains(t)) {
            return false;
        }

        // Fill the hit record
        rec.t = t;
        rec.p = r.at(t);

        rec.normal = normal;

        rec.material = &material;
        return true;
    }

private:
    // Vertices
    point3 v0, v1, v2;

    // Precomputed edges
    vec3 edge01, edge02;

    vec3 normal;

    const mat& material;

    // Helper function to recalc edges and normals
    void update_triangle() {
        edge01 = v1 - v0;
        edge02 = v2 - v0;
        normal = unit_vector(cross(edge01, edge02));
    }
};

#endif
