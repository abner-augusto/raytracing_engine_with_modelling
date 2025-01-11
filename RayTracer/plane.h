#ifndef PLANE_H
#define PLANE_H
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "matrix4x4.h"

class plane : public hittable {
public:
    plane(const point3& point_on_plane, const vec3& normal_vector, const mat& material,
        double scale_factor = 1.0)  // Add scale factor parameter
        : point(point_on_plane), normal(unit_vector(normal_vector)),
        material(material), scale(scale_factor) {
    }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Compute the denominator of the intersection formula
        auto denom = dot(normal, r.direction());

        // If the denominator is close to zero, the ray is parallel to the plane
        if (fabs(denom) < 1e-6) {
            return false;
        }

        // Calculate the intersection point
        auto t = dot(point - r.origin(), normal) / denom;

        // Check if t is within the valid range
        if (!ray_t.surrounds(t)) {
            return false;
        }
        rec.t = t;
        rec.p = r.at(rec.t);
        rec.set_face_normal(r, normal);
        rec.material = &material;

        // Calculate UV coordinates
        calculate_uv(rec.p, rec.u, rec.v);
        return true;
    }

    void calculate_uv(const point3& hit_point, double& u, double& v) const {
        // Scale the local point coordinates before UV mapping
        vec3 local_point = hit_point - point;
        // Apply the scale factor here
        u = fmod(local_point.x() * scale, 1.0);
        v = fmod(local_point.z() * scale, 1.0);

        // Ensure UV are positive
        if (u < 0) u += 1.0;
        if (v < 0) v += 1.0;
    }

    void transform(const Matrix4x4& matrix) override {
        point = matrix.transform_point(point);
        normal = matrix.transform_vector(normal);
        normal = unit_vector(normal);
    }

private:
    point3 point;
    vec3 normal;
    mat material;
    double scale;
};
#endif