#ifndef CONE_H
#define CONE_H

#include <cmath>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "matrix4x4.h"

class cone : public hittable {
public:
    cone(const point3& base_center, const point3& top_vertex, double radius, const mat& material, bool capped = true)
        : base_center(base_center), top_vertex(top_vertex), radius(std::fmax(0, radius)), material(material), capped(capped) {
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

    void set_capped(bool is_capped) {
        capped = is_capped;
    }

    bool hit(const ray& ray, interval ray_t, hit_record& record) const override {
        vec3 ray_origin = ray.origin();
        vec3 ray_direction = ray.direction();
        vec3 cone_to_origin = ray_origin - top_vertex; // Vector from cone apex to ray origin

        // Precompute reusable dot products
        double axis_dot_direction = dot(ray_direction, axis_direction);
        double axis_dot_cone_to_origin = dot(cone_to_origin, axis_direction);
        double direction_dot_cone_to_origin = dot(ray_direction, cone_to_origin);
        double cone_to_origin_dot_cone_to_origin = dot(cone_to_origin, cone_to_origin);

        // Calculate quadratic coefficients
        double a = axis_dot_direction * axis_dot_direction - cos_angle_sq;
        double b = 2.0 * (axis_dot_direction * axis_dot_cone_to_origin - direction_dot_cone_to_origin * cos_angle_sq);
        double c = axis_dot_cone_to_origin * axis_dot_cone_to_origin - cone_to_origin_dot_cone_to_origin * cos_angle_sq;

        // Compute the discriminant
        double discriminant = b * b - 4.0 * a * c;
        if (discriminant < 0.0) return false; // No real solutions, no intersection

        double sqrt_discriminant = std::sqrt(discriminant);
        double t1 = (-b - sqrt_discriminant) / (2.0 * a);
        double t2 = (-b + sqrt_discriminant) / (2.0 * a);

        // Helper function to check if t value hits the cone
        auto is_valid_cone_hit = [&](double t) {
            if (t <= 0.0 || !ray_t.contains(t)) return false;
            vec3 intersection_to_apex = (ray_origin + t * ray_direction) - top_vertex;
            double height_at_t = dot(intersection_to_apex, axis_direction);
            return height_at_t >= 0.0 && height_at_t <= height;
            };

        bool hit_detected = false;
        double t = 0.0;
        vec3 surface_normal;

        // Check intersections with the cone surface
        if (is_valid_cone_hit(t1)) {
            hit_detected = true;
            t = t1;
            vec3 intersection_to_apex = (ray_origin + t * ray_direction) - top_vertex;
            double height_at_intersection = dot(intersection_to_apex, axis_direction);
            surface_normal = unit_vector(intersection_to_apex - axis_direction * height_at_intersection);
        }
        if (is_valid_cone_hit(t2) && (!hit_detected || t2 < t)) {
            hit_detected = true;
            t = t2;
            vec3 intersection_to_apex = (ray_origin + t * ray_direction) - top_vertex;
            double height_at_intersection = dot(intersection_to_apex, axis_direction);
            surface_normal = unit_vector(intersection_to_apex - axis_direction * height_at_intersection);
        }

        // Check intersection with the base if necessary
        if (capped && (!hit_detected || ray_t.contains(t))) {
            double denominator = dot(ray_direction, axis_direction);
            if (std::fabs(denominator) > 1e-8) {
                double t_base = dot(base_center - ray_origin, axis_direction) / denominator;
                if (ray_t.contains(t_base) && t_base > 0.0) {
                    point3 base_intersection = ray.at(t_base);
                    if ((base_intersection - base_center).length_squared() <= radius * radius) {
                        t = t_base;
                        surface_normal = -axis_direction;
                        hit_detected = true;
                    }
                }
            }
        }

        if (!hit_detected) return false;

        // Fill the hit record with intersection details
        record.t = t;
        record.p = ray.at(t);
        record.set_face_normal(ray, surface_normal);
        record.material = &material;
        return true;
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

private:
    point3 base_center;
    point3 top_vertex;
    vec3 axis_direction;
    double height;
    double radius;
    double cos_angle;
    double cos_angle_sq;

    mat material;
    bool capped;

    void update_constants() {
        vec3 axis = base_center - top_vertex; // Define o eixo como top_vertex - base_center
        height = axis.length();               // Calcula a altura como a distância entre base e vértice
        axis_direction = unit_vector(axis);   // Direção do eixo (unitário)
        cos_angle = height / std::sqrt(height * height + radius * radius); // Calcula o ângulo
        cos_angle_sq = cos_angle * cos_angle; // Pré-calcula o cosseno ao quadrado
    }

};

#endif
