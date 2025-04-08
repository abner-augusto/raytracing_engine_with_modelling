#ifndef SPHERE_H
#define SPHERE_H

#include <algorithm>
#include <cmath>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "boundingbox.h"

class sphere : public hittable {
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

    void set_material(const mat& new_material) override {
        material = new_material;
    }

    mat get_material() const override {
        return material;
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
        rec.hit_object = this;

        // Calculate UV coordinates
        calculate_uv((rec.p - center) / radius, rec.u, rec.v);

        return true;
    }

    bool csg_intersect(const ray& r, interval ray_t,
        std::vector<CSGIntersection>& out_intersections) const override {
        out_intersections.clear();

        // Calculate basic intersection parameters
        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;
        auto discriminant = half_b * half_b - a * c;

        if (discriminant < 0) return false;  // No intersections

        auto sqrtd = std::sqrt(discriminant);
        auto root1 = (-half_b - sqrtd) / a;
        auto root2 = (-half_b + sqrtd) / a;

        // Add a small epsilon for numerical stability
        const double eps = 1e-12;

        // Check if the roots are within our valid interval
        bool hit1 = ray_t.surrounds(root1);
        bool hit2 = ray_t.surrounds(root2);

        if (!hit1 && !hit2) return false;

        // Determine if the ray starts inside the sphere
        bool ray_starts_inside = oc.length_squared() < (radius * radius + eps);

        // Always add both intersections when they're valid, even if outside ray_t
        // This helps with CSG operations that might need to know about all intersections
        if (std::isfinite(root1)) {
            vec3 hit_point = r.at(root1);
            vec3 normal = (hit_point - center) / radius;
            // Entry point: normal points inward if ray starts inside
            if (ray_starts_inside) normal = -normal;
            out_intersections.emplace_back(
                root1,
                !ray_starts_inside,  // is_entry is opposite of ray_starts_inside
                this,
                normal,
                hit_point
            );
        }

        if (std::isfinite(root2)) {
            vec3 hit_point = r.at(root2);
            vec3 normal = (hit_point - center) / radius;
            // Exit point: normal points outward if ray starts inside
            if (!ray_starts_inside) normal = -normal;
            out_intersections.emplace_back(
                root2,
                ray_starts_inside,  // is_entry same as ray_starts_inside
                this,
                normal,
                hit_point
            );
        }

        // Sort intersections by t value (important for CSG operations)
        std::sort(out_intersections.begin(), out_intersections.end(),
            [](const CSGIntersection& a, const CSGIntersection& b) {
                return a.t < b.t;
            });

        return !out_intersections.empty();
    }

    void calculate_uv(const vec3& normal, double& u, double& v) const {
        // Convert normal to spherical coordinates
        auto theta = std::acos(-normal.y())*0.5; // Latitude
        auto phi = std::atan2(-normal.z(), normal.x()) + M_PI; // Longitude

        // Normalize to [0, 1] range
        u = phi / (2 * M_PI);
        v = theta / M_PI;
    }

    bool is_point_inside(const point3& p) const override {
        return (p - center).length() <= radius;
    }

    // Returns:
    // 'w' if the bounding box doesn't intersect the sphere (empty)
    // 'b' if the bounding box is completely inside the sphere (full)
    // 'g' otherwise (partial)
    char test_bb(const BoundingBox& bb) const override {
        point3 closest = bb.getClosestPoint(center);
        if (!is_point_inside(closest)) {
            return 'w';
        }

        point3 furthest = bb.getFurthestPoint(center);
        if (is_point_inside(furthest)) {
            return 'b';
        }

        return 'g';
    }

    void transform(const Matrix4x4& matrix) override {
        // Apply transformation to the center of the sphere
        center = matrix.transform_point(center);

        // Apply uniform scaling radius of the sphere
        double scalingFactor = matrix.get_uniform_scale();
        radius *= scalingFactor;
    }

    BoundingBox bounding_box() const override {
        // Calculate the min and max points for the bounding box
        point3 min_point = center - vec3(radius, radius, radius);
        point3 max_point = center + vec3(radius, radius, radius);

        return BoundingBox(min_point, max_point);
    }

    std::string get_type_name() const override {
        return "Sphere";
    }

    std::shared_ptr<hittable> clone() const override {
        return std::make_shared<sphere>(*this);
    }

private:
    point3 center;
    double radius;
    mat material;
};

#endif
