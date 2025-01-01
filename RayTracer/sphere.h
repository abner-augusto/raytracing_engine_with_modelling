#ifndef SPHERE_H
#define SPHERE_H

#include <algorithm>
#include <cmath>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "boundingbox.h"
#include "primitive.h"

class sphere : public hittable, public Primitive {
public:
    sphere(const point3& center, double radius, const mat& material)
        : center(center), radius(std::fmax(0, radius)), material(material) {
    }

    void set_center(const point3& new_center) {
        center = new_center;
    }

    void set_radius(double new_radius) {
        radius = std::fmax(0, new_radius);
    }

    void set_material(const mat& new_material) {
        material = new_material;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;

        auto discriminant = half_b * half_b - a * c;
        if (discriminant < 0)
            return false;

        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (-half_b - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (-half_b + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.material = &material;

        // Calculate UV coordinates
        calculate_uv((rec.p - center) / radius, rec.u, rec.v);

        return true;
    }

    void calculate_uv(const vec3& normal, double& u, double& v) const {
        // Convert normal to spherical coordinates
        auto theta = std::acos(-normal.y())*0.5; // Latitude
        auto phi = std::atan2(-normal.z(), normal.x()) + M_PI; // Longitude

        // Normalize to [0, 1] range
        u = phi / (2 * M_PI);
        v = theta / M_PI;
    }

    // Additional methods for Octree usage
    bool test_point(const point3& p) const {
        return distance(p, center) <= radius;
    }

    // Returns:
    // 'w' if the bounding box doesn't intersect the sphere (empty)
    // 'b' if the bounding box is completely inside the sphere (full)
    // 'g' otherwise (partial)
    char test_bb(const BoundingBox& bb) const {
        point3 closest = bb.ClosestPoint(center);
        if (!test_point(closest)) {
            return 'w';
        }

        point3 furthest = bb.FurthestPoint(center);
        if (test_point(furthest)) {
            return 'b';
        }

        return 'g';
    }

private:
    point3 center;
    double radius;
    mat material;
};

#endif
