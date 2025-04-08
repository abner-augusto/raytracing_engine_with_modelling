#ifndef PLANE_H
#define PLANE_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "matrix4x4.h"

class plane : public hittable {
public:
    plane(const point3& point_on_plane, const vec3& normal_vector, const mat& material,
        double scale_factor = 1.0)
        : point(point_on_plane), normal(unit_vector(normal_vector)),
        material(material), scale(scale_factor) {
        // Compute local axes for the plane
        vec3 t = fabs(normal.x()) > 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0);
        u_axis = unit_vector(cross(normal, t));
        v_axis = unit_vector(cross(normal, u_axis));
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
        rec.hit_object = this;

        // Calculate UV coordinates
        calculate_uv(rec.p, rec.u, rec.v);
        return true;
    }


    void calculate_uv(const point3& hit_point, double& u, double& v) const {
        // Compute vector from reference point to hit point
        vec3 local_vec = hit_point - point;

        u = dot(local_vec, u_axis) * scale;
        v = dot(local_vec, v_axis) * scale;

    }


    void transform(const Matrix4x4& matrix) override {
        point = matrix.transform_point(point);
        normal = matrix.transform_vector(normal);
        normal = unit_vector(normal);

        // Recalculate local axes after transformation
        vec3 t = fabs(normal.x()) > 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0);
        u_axis = unit_vector(cross(normal, t));
        v_axis = unit_vector(cross(normal, u_axis));
    }

    BoundingBox bounding_box() const override {
        // Define an arbitrary large extent for the plane in the x and z directions
        const double extent = 1e6;

        // Use the plane's point for the y-coordinate and extend x and z
        point3 min_point(-extent, point.y() - 0.01, -extent);
        point3 max_point(extent, point.y() + 0.01, extent);

        return BoundingBox(min_point, max_point);
    }

    std::string get_type_name() const override {
        return "Plane";
    }

    void set_material(const mat& new_material) override {
        material = new_material;
    }

    mat get_material() const override {
        return material;
    }

    std::shared_ptr<hittable> clone() const override {
        return std::make_shared<plane>(*this);
    }

private:
    point3 point;  // A point on the plane
    vec3 normal;   // The plane's normal vector
    vec3 u_axis;   // Local U axis
    vec3 v_axis;   // Local V axis
    mat material;  // Material for the plane
    double scale;  // Scale factor for UV mapping
};

#endif
