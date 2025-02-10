#ifndef TORUS_H
#define TORUS_H

#include <cmath>
#include <algorithm>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "matrix4x4.h"

// Computes intersection of a ray with a torus
// ro: ray origin (in object space)
// rd: ray direction (in object space)
// major_radius: Radius from the torus center to the tube center
// minor_radius: Radius of the tube itself
static double compute_torus_intersection(const vec3& ray_origin, const vec3& ray_direction, double major_radius, double minor_radius) {
    double majorR = major_radius;
    double minorR = minor_radius;
    double majorRadiusSquared = majorR * majorR;
    double minorRadiusSquared = minorR * minorR;

    double origin_dot = dot(ray_origin, ray_origin);
    double origin_dir_dot = dot(ray_origin, ray_direction);
    double constant_term = (origin_dot + majorRadiusSquared - minorRadiusSquared) / 2.0;

    double cubic_term = origin_dir_dot;
    double quadratic_term = origin_dir_dot * origin_dir_dot - majorRadiusSquared * (ray_direction[0] * ray_direction[0] + ray_direction[1] * ray_direction[1]) + constant_term;
    double linear_term = origin_dir_dot * constant_term - majorRadiusSquared * (ray_direction[0] * ray_origin[0] + ray_direction[1] * ray_origin[1]);
    double constant_term_final = constant_term * constant_term - majorRadiusSquared * (ray_origin[0] * ray_origin[0] + ray_origin[1] * ray_origin[1]);

    double sign_adjustment = 1.0;
    if (std::fabs(cubic_term * (cubic_term * cubic_term - quadratic_term) + linear_term) < 0.01) {
        sign_adjustment = -1.0;
        std::swap(cubic_term, linear_term);
        constant_term_final = 1.0 / constant_term_final;
        linear_term *= constant_term_final;
        quadratic_term *= constant_term_final;
        cubic_term *= constant_term_final;
    }

    double adjusted_quadratic = quadratic_term * 2.0 - 3.0 * cubic_term * cubic_term;
    double adjusted_linear = cubic_term * (cubic_term * cubic_term - quadratic_term) + linear_term;
    double adjusted_constant = cubic_term * (cubic_term * (adjusted_quadratic + 2.0 * quadratic_term) - 8.0 * linear_term) + 4.0 * constant_term_final;

    adjusted_quadratic /= 3.0;
    adjusted_linear *= 2.0;
    adjusted_constant /= 3.0;

    double Q = adjusted_quadratic * adjusted_quadratic + adjusted_constant;
    double adjustedR = adjusted_quadratic * adjusted_quadratic * adjusted_quadratic - 3.0 * adjusted_quadratic * adjusted_constant + adjusted_linear * adjusted_linear;
    double discriminant = adjustedR * adjustedR - Q * Q * Q;

    auto sign = [](double x) { return (x > 0.0) ? 1.0 : (x < 0.0 ? -1.0 : 0.0); };

    double t = -1.0;

    if (discriminant >= 0.0) {
        double sqrt_discriminant = std::sqrt(discriminant);
        double v = sign(adjustedR + sqrt_discriminant) * std::pow(std::fabs(adjustedR + sqrt_discriminant), 1.0 / 3.0);
        double u = sign(adjustedR - sqrt_discriminant) * std::pow(std::fabs(adjustedR - sqrt_discriminant), 1.0 / 3.0);
        double s[2] = { (v + u) + 4.0 * adjusted_quadratic, (v - u) * std::sqrt(3.0) };
        double y = std::sqrt(0.5 * (std::sqrt(s[0] * s[0] + s[1] * s[1]) + s[0]));
        double x = 0.5 * s[1] / y;
        double r = 2.0 * adjusted_linear / (x * x + y * y);
        double t1 = x - r - cubic_term;
        t1 = (sign_adjustment < 0.0) ? 2.0 / t1 : t1;
        double t2 = -x - r - cubic_term;
        t2 = (sign_adjustment < 0.0) ? 2.0 / t2 : t2;

        double t_min = 1e20;
        if (t1 > 0.0) t_min = t1;
        if (t2 > 0.0) t_min = std::min(t_min, t2);
        t = (t_min < 1e20) ? t_min : -1.0;
    }
    else {
        double sqrt_Q = std::sqrt(Q);
        double w = sqrt_Q * std::cos(std::acos(-adjustedR / (sqrt_Q * Q)) / 3.0);
        double delta_2 = -(w + adjusted_quadratic);
        if (delta_2 < 0.0) return -1.0;
        double delta_1 = std::sqrt(delta_2);
        double h1 = std::sqrt(w - 2.0 * adjusted_quadratic + adjusted_linear / delta_1);
        double h2 = std::sqrt(w - 2.0 * adjusted_quadratic - adjusted_linear / delta_1);

        double t1 = -delta_1 - h1 - cubic_term;
        t1 = (sign_adjustment < 0.0) ? 2.0 / t1 : t1;
        double t2 = -delta_1 + h1 - cubic_term;
        t2 = (sign_adjustment < 0.0) ? 2.0 / t2 : t2;
        double t3 = delta_1 - h2 - cubic_term;
        t3 = (sign_adjustment < 0.0) ? 2.0 / t3 : t3;
        double t4 = delta_1 + h2 - cubic_term;
        t4 = (sign_adjustment < 0.0) ? 2.0 / t4 : t4;

        double t_min = 1e20;
        if (t1 > 0.0) t_min = t1;
        if (t2 > 0.0) t_min = std::min(t_min, t2);
        if (t3 > 0.0) t_min = std::min(t_min, t3);
        if (t4 > 0.0) t_min = std::min(t_min, t4);

        t = (t_min < 1e20) ? t_min : -1.0;
    }

    return t;
}

// Computes the surface normal at a given point on the torus
// pos: Position on the torus (in object space)
// major_radius: Radius from the torus center to the tube center
// minor_radius: Radius of the tube itself
static vec3 compute_torus_normal(const vec3& position, double major_radius, double minor_radius) {
    double majorR = major_radius;

    double X = position[0];
    double Y = position[1];
    double Z = position[2];

    double distance = std::sqrt(X * X + Y * Y);

    if (distance < 1e-14) {
        return unit_vector(vec3(X, Y, Z));
    }

    vec3 gradient(
        2.0 * (distance - majorR) * (X / distance),
        2.0 * (distance - majorR) * (Y / distance),
        2.0 * Z
    );

    return unit_vector(gradient);
}

class torus : public hittable {
public:
    torus(const point3& center, double major_radius, double minor_radius, const vec3& axis_direction, const mat& material)
        : center(center),
        major_radius(std::fmax(0.0, major_radius)),
        minor_radius(std::fmax(0.0, minor_radius)),
        material(material) {
        w = unit_vector(axis_direction);
        vec3 arbitrary = (std::fabs(w[0]) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
        u = unit_vector(cross(arbitrary, w));
        v = cross(w, u);
    }

    void set_center(const point3& c) {
        center = c;
    }

    void set_major_radius(double R) {
        major_radius = std::fmax(0.0, R);
    }

    void set_minor_radius(double r) {
        minor_radius = std::fmax(0.0, r);
    }

    void set_axis_direction(const vec3& dir) {
        w = unit_vector(dir);
        vec3 arbitrary = (std::fabs(w[0]) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
        u = unit_vector(cross(arbitrary, w));
        v = cross(w, u);
    }

    void set_material(const mat& m) {
        material = m;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Translate ray origin to the torus's local coordinate system
        vec3 ray_origin_object_space = r.origin() - center;
        vec3 ray_direction_object_space = r.direction();

        // Convert ray to the torus's local object space using basis vectors
        vec3 local_origin(
            dot(ray_origin_object_space, u),
            dot(ray_origin_object_space, v),
            dot(ray_origin_object_space, w)
        );
        vec3 local_direction(
            dot(ray_direction_object_space, u),
            dot(ray_direction_object_space, v),
            dot(ray_direction_object_space, w)
        );

        // Compute the intersection of the ray with the torus
        double t = compute_torus_intersection(local_origin, local_direction, major_radius, minor_radius);

        // Check if there is a valid intersection within the ray's bounds
        if (t < 0.0 || !ray_t.contains(t))
            return false;

        // Populate the hit record with intersection data
        rec.t = t;
        rec.p = r.at(t);

        // Compute the surface normal at the hit point in local space
        vec3 hit_point_local = local_origin + t * local_direction;
        vec3 normal_local = compute_torus_normal(hit_point_local, major_radius, minor_radius);

        // Transform the normal back to world space
        vec3 normal_world = normal_local[0] * u + normal_local[1] * v + normal_local[2] * w;

        rec.set_face_normal(r, normal_world);
        rec.material = &material;
        rec.hit_object = this;

        return true;
    }

    void transform(const Matrix4x4& matrix) override {
        center = matrix.transform_point(center);

        u = matrix.transform_vector(u);
        v = matrix.transform_vector(v);
        w = matrix.transform_vector(w);

        w = unit_vector(w);
        u = unit_vector(cross(vec3(0, 1, 0), w));
        v = cross(w, u);

        // Adjust radii for uniform scaling (if applicable)
        /*double scale_factor = matrix.get_uniform_scale();
        major_radius *= scale_factor;
        minor_radius *= scale_factor;*/
    }

    BoundingBox bounding_box() const override {
        double max_extent = major_radius + minor_radius;

        point3 min_point = center - vec3(max_extent, max_extent, max_extent);
        point3 max_point = center + vec3(max_extent, max_extent, max_extent);

        return BoundingBox(min_point, max_point);
    }

    std::string get_type_name() const override {
        return "Torus";
    }

private:
    point3 center;
    double major_radius;
    double minor_radius;
    mat material;

    vec3 u, v, w;
};

#endif
