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

    bool hit_all(const ray& r, interval ray_t, std::vector<hit_record>& recs) const override {
        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;
        auto discriminant = half_b * half_b - a * c;

        if (discriminant < 0) return false;

        auto sqrtd = sqrt(discriminant);
        auto root1 = (-half_b - sqrtd) / a;
        auto root2 = (-half_b + sqrtd) / a;

        // Check if either intersection point is within our interval
        bool hit1 = ray_t.surrounds(root1);
        bool hit2 = ray_t.surrounds(root2);

        if (!hit1 && !hit2) return false;

        // Entry point
        if (hit1) {
            hit_record rec;
            rec.t = root1;
            rec.p = r.at(rec.t);
            vec3 outward_normal = (rec.p - center) / radius;
            rec.set_face_normal(r, outward_normal);
            rec.material = &material;
            rec.is_entry = true;
            calculate_uv((rec.p - center) / radius, rec.u, rec.v);
            recs.push_back(rec);
        }

        // Exit point
        if (hit2) {
            hit_record rec;
            rec.t = root2;
            rec.p = r.at(rec.t);
            vec3 outward_normal = (rec.p - center) / radius;
            rec.set_face_normal(r, outward_normal);
            rec.material = &material;
            rec.is_entry = false;
            calculate_uv((rec.p - center) / radius, rec.u, rec.v);
            recs.push_back(rec);
        }

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
        point3 closest = bb.getClosestPoint(center);
        if (!test_point(closest)) {
            return 'w';
        }

        point3 furthest = bb.getFurthestPoint(center);
        if (test_point(furthest)) {
            return 'b';
        }

        return 'g';
    }

    void transform(const Matrix4x4& matrix) override {
        // Apply transformation to the center of the sphere
        center = matrix.transform_point(center);

        // Apply uniform scaling radius of the sphere
        /*double scalingFactor = matrix.get_uniform_scale();
        radius *= scalingFactor;*/
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

private:
    point3 center;
    double radius;
    mat material;
};

#endif
