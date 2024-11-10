#ifndef PLANE_H
#define PLANE_H

#include "hittable.h"
#include "vec3.h"

class plane : public hittable {
public:
    plane(const point3& point_on_plane, const vec3& normal_vector, const color& plane_color)
        : point(point_on_plane), normal(unit_vector(normal_vector)), plane_color(plane_color) {}

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Compute the denominator of the intersection formula
        auto denom = dot(normal, r.direction());

        // If the denominator is close to zero, the ray is parallel to the plane
        if (fabs(denom) < 1e-6) {
            return false;
        }

        // Compute the intersection point
        auto t = dot(point - r.origin(), normal) / denom;

        // Check if t is within the valid interval
        if (!ray_t.surrounds(t)) {
            return false;
        }

        rec.t = t;
        rec.p = r.at(rec.t);
        rec.set_face_normal(r, normal);
        rec.obj_color = plane_color;

        return true;
    }

private:
    point3 point;       // A point on the plane
    vec3 normal;        // The plane's normal vector (should be normalized)
    color plane_color;  // The plane's color
};

#endif
