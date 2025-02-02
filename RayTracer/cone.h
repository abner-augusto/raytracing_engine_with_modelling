#ifndef CONE_H
#define CONE_H

#include <cmath>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "matrix4x4.h"

class cone : public hittable {
public:
    cone(const point3& base_center, const point3& top_vertex, double radius, const mat& material)
        : base_center(base_center), top_vertex(top_vertex), radius(std::fmax(0, radius)), material(material) {
        update_constants();
    }

    void set_base_center(const point3& new_base_center) {
        base_center = new_base_center;
        update_constants();
    }

    void set_top_vertex(const point3& new_top_vertex) {
        top_vertex = new_top_vertex;
        update_constants();
    }

    void set_radius(double new_radius) {
        radius = std::fmax(0, new_radius);
        update_constants();
    }

    void set_material(const mat& new_material) {
        material = new_material;
    }

    bool hit(const ray& ray, interval ray_t, hit_record& record) const override {
        vec3 ray_origin = ray.origin();
        vec3 ray_direction = ray.direction();
        vec3 cone_to_origin = ray_origin - top_vertex;

        // Precompute reusable dot products
        double axis_dot_direction = dot(ray_direction, axis_direction);
        double axis_dot_cone_to_origin = dot(cone_to_origin, axis_direction);
        double direction_dot_cone_to_origin = dot(ray_direction, cone_to_origin);
        double cone_to_origin_dot_cone_to_origin = dot(cone_to_origin, cone_to_origin);

        // Calculate quadratic coefficients
        double a = axis_dot_direction * axis_dot_direction - cos_angle_sq;
        double b = 2.0 * (axis_dot_direction * axis_dot_cone_to_origin - direction_dot_cone_to_origin * cos_angle_sq);
        double c = axis_dot_cone_to_origin * axis_dot_cone_to_origin - cone_to_origin_dot_cone_to_origin * cos_angle_sq;

        // Handle linear case if a is near zero
        const double EPS = 1e-8;
        bool hit_detected = false;
        double t = 0.0;
        vec3 surface_normal;

        if (std::fabs(a) < EPS) {
            // Linear equation: b*t + c = 0
            if (std::fabs(b) < EPS) return false;
            double t_linear = -c / b;
            if (is_valid_cone_hit(t_linear, ray_origin, ray_direction, ray_t)) {
                hit_detected = true;
                t = t_linear;
                update_surface_normal(t, ray_origin, ray_direction, surface_normal);
            }
        }
        else {
            // Quadratic equation
            double discriminant = b * b - 4.0 * a * c;
            if (discriminant < 0.0) return false;

            double sqrt_discriminant = std::sqrt(discriminant);
            double t1 = (-b - sqrt_discriminant) / (2.0 * a);
            double t2 = (-b + sqrt_discriminant) / (2.0 * a);

            // Check intersections with the cone surface
            if (is_valid_cone_hit(t1, ray_origin, ray_direction, ray_t)) {
                hit_detected = true;
                t = t1;
                update_surface_normal(t, ray_origin, ray_direction, surface_normal);
            }
            if (is_valid_cone_hit(t2, ray_origin, ray_direction, ray_t) && (!hit_detected || t2 < t)) {
                hit_detected = true;
                t = t2;
                update_surface_normal(t, ray_origin, ray_direction, surface_normal);
            }
        }

        if (!hit_detected) return false;

        // Fill hit record
        record.t = t;
        record.p = ray.at(t);
        record.set_face_normal(ray, surface_normal);
        record.material = &material;
        record.hit_object = this;
        return true;
    }

    bool csg_intersect(const ray& r, interval ray_t,
        std::vector<CSGIntersection>& out_intersections) const override {
        out_intersections.clear();
        const vec3 ray_origin = r.origin();
        const vec3 ray_direction = r.direction();
        const double EPS = 1e-7;

        // Lateral surface intersection
        vec3 v = ray_origin - top_vertex;
        double axis_dot_direction = dot(ray_direction, axis_direction);
        double axis_dot_v = dot(v, axis_direction);
        double direction_dot_v = dot(ray_direction, v);
        double v_dot_v = dot(v, v);

        double a = axis_dot_direction * axis_dot_direction - cos_angle_sq;
        double b = 2.0 * (axis_dot_direction * axis_dot_v - direction_dot_v * cos_angle_sq);
        double c = axis_dot_v * axis_dot_v - v_dot_v * cos_angle_sq;

        std::vector<std::pair<double, vec3>> hits;

        if (std::fabs(a) > EPS) {
            double discriminant = b * b - 4.0 * a * c;
            if (discriminant >= 0.0) {
                double sqrt_disc = std::sqrt(discriminant);
                double t1 = (-b - sqrt_disc) / (2.0 * a);
                double t2 = (-b + sqrt_disc) / (2.0 * a);

                auto valid_cone_hit = [&](double t) -> bool {
                    if (t < ray_t.min || t > ray_t.max) return false;
                    vec3 p_hit = r.at(t);
                    vec3 v_hit = p_hit - top_vertex;
                    double proj = dot(v_hit, axis_direction);
                    return (proj >= 0.0 && proj <= height);
                    };

                if (valid_cone_hit(t1)) {
                    vec3 p_hit = r.at(t1);
                    vec3 v_hit = p_hit - top_vertex;
                    double proj = dot(v_hit, axis_direction);
                    vec3 normal = unit_vector(v_hit * cos_angle_sq - axis_direction * proj);
                    hits.emplace_back(t1, normal);
                }
                if (valid_cone_hit(t2)) {
                    vec3 p_hit = r.at(t2);
                    vec3 v_hit = p_hit - top_vertex;
                    double proj = dot(v_hit, axis_direction);
                    vec3 normal = unit_vector(v_hit * cos_angle_sq - axis_direction * proj);
                    hits.emplace_back(t2, normal);
                }
            }
        }

        // Sort hits and determine entry/exit
        std::sort(hits.begin(), hits.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        bool ray_starts_inside = IsPointInside(ray_origin);
        for (const auto& [t_val, normal] : hits) {
            if (t_val < ray_t.min || t_val > ray_t.max) continue;

            bool is_entry = ray_starts_inside ? !out_intersections.empty() : out_intersections.empty();
            vec3 adjusted_normal = is_entry ? normal : -normal;

            out_intersections.emplace_back(
                t_val, is_entry, this, adjusted_normal, r.at(t_val)
            );
        }

        return !out_intersections.empty();
    }

    // Inside test for the cone.
    // A point is considered inside if its projection along the axis (from the apex)
    // is between 0 and height and if its radial distance is less than or equal to the
    // interpolated radius at that height.
    bool IsPointInside(const point3& p) const {
        // Compute vector from the apex (top_vertex) to p.
        vec3 v = p - top_vertex;
        double proj = dot(v, axis_direction);
        if (proj < 0.0 || proj > height)
            return false;

        // Find the point on the cone's axis corresponding to the projection.
        point3 p_on_axis = top_vertex + proj * axis_direction;
        double dist_sq = (p - p_on_axis).length_squared();

        // The maximum allowed radius at this height (linearly from 0 at the apex to 'radius' at the base)
        double max_radius = (proj / height) * radius;
        return dist_sq <= max_radius * max_radius;
    }

    void transform(const Matrix4x4& matrix) override {
        // Transform the base center and top vertex
        base_center = matrix.transform_point(base_center);
        top_vertex = matrix.transform_point(top_vertex);

        // Transform the axis direction
        axis_direction = matrix.transform_vector(axis_direction);
        axis_direction = unit_vector(axis_direction); // Re-normalize the axis direction

        // Adjust the radius and height for scaling (assuming uniform scaling)
        /*double scale_factor = matrix.get_uniform_scale();
        radius *= scale_factor;*/

        // Recalculate height and other constants
        update_constants();
    }

    BoundingBox bounding_box() const override {
        // Find the axis-aligned bounding box of the cone
        vec3 radius_vector(radius, radius, radius);

        // Compute the min and max points for the bounding box
        point3 min_point(
            std::min(base_center.x() - radius, top_vertex.x()),
            std::min(base_center.y() - radius, top_vertex.y()),
            std::min(base_center.z() - radius, top_vertex.z())
        );

        point3 max_point(
            std::max(base_center.x() + radius, top_vertex.x()),
            std::max(base_center.y() + radius, top_vertex.y()),
            std::max(base_center.z() + radius, top_vertex.z())
        );

        return BoundingBox(min_point, max_point);
    }

    std::string get_type_name() const override {
        return "Cone";
    }

private:
    point3 base_center;
    point3 top_vertex;
    vec3 axis_direction;
    double height;
    double radius;
    double cos_angle;
    double cos_angle_sq;
    mat material;

    void update_constants() {
        vec3 axis = base_center - top_vertex;
        height = axis.length();
        axis_direction = unit_vector(axis);
        cos_angle = height / std::sqrt(height * height + radius * radius);
        cos_angle_sq = cos_angle * cos_angle;
    }

    bool is_valid_cone_hit(double t, const vec3& ro, const vec3& rd, const interval& ray_t) const {
        if (!ray_t.contains(t)) return false;
        vec3 intersection_to_apex = ro + t * rd - top_vertex;
        double h = dot(intersection_to_apex, axis_direction);
        return h >= 0.0 && h <= height;
    }

    void update_surface_normal(double t, const vec3& ro, const vec3& rd, vec3& normal) const {
        vec3 intersection_to_apex = ro + t * rd - top_vertex;
        double h = dot(intersection_to_apex, axis_direction);
        vec3 gradient = (intersection_to_apex * cos_angle_sq) - (axis_direction * h);
        normal = unit_vector(gradient);
    }
};

#endif
