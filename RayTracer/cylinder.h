#ifndef CYLINDER_H
#define CYLINDER_H

#include <cmath>
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "boundingbox.h"
#include "matrix4x4.h"

class cylinder : public hittable {
public:
    cylinder(const point3& base_center, double height, double radius, const mat& material, bool capped = true)
        : base_center(base_center),
        radius(std::fmax(0, radius)),
        material(material),
        capped(capped),
        height(height)
    {
        top_center = base_center + vec3(0, height, 0);
        update_constants();
    }

    cylinder(const point3& base_center, const point3& top_center, double radius, const mat& material, bool capped = true)
        : base_center(base_center), top_center(top_center), radius(std::fmax(0, radius)), material(material), capped(capped)
    {
        height = (top_center - base_center).length();
        update_constants();
    }

    cylinder(const point3& base_center, double height, const vec3& direction, double radius, const mat& material, bool capped = true)
        : base_center(base_center),
        radius(std::fmax(0, radius)),
        material(material),
        capped(capped),
        height(height)
    {
        vec3 normalized_direction = unit_vector(direction);
        top_center = base_center + normalized_direction * height;
        update_constants();
    }

    void set_base_center(const point3& new_base_center) {
        base_center = new_base_center;
        top_center = base_center + vec3(0, height, 0);
        update_constants();
    }

    void set_height(double new_height) {
        height = new_height;
        top_center = base_center + vec3(0, height, 0);
        update_constants();
    }

    void set_top_center(const point3& new_top_center) {
        top_center = new_top_center;
        height = (top_center - base_center).length();
        update_constants();
    }

    void set_radius(double new_radius) {
        radius = std::fmax(0, new_radius);
        update_constants();
    }

    void set_material(const mat& new_material) {
        material = new_material;
    }

    void set_capped(bool new_capped) {
        capped = new_capped;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 ray_origin = r.origin();
        vec3 ray_direction = r.direction();
        vec3 origin_to_cylinder_start = ray_origin - base_center;
        const double epsilon = 1e-6;

        // Precompute dot products for the ray and cylinder axis
        double axis_dot_direction = dot(unit_cylinder_axis, ray_direction);
        double axis_dot_origin_to_start = dot(unit_cylinder_axis, origin_to_cylinder_start);

        // Solve the quadratic equation for intersection with the infinite cylinder
        double quadratic_a = 1.0 - axis_dot_direction * axis_dot_direction;
        double quadratic_b = dot(origin_to_cylinder_start, ray_direction) - axis_dot_origin_to_start * axis_dot_direction;
        double quadratic_c = dot(origin_to_cylinder_start, origin_to_cylinder_start) - axis_dot_origin_to_start * axis_dot_origin_to_start - radius_squared;

        double discriminant = quadratic_b * quadratic_b - quadratic_a * quadratic_c;
        if (discriminant < 0.0)
            return false;

        discriminant = std::sqrt(discriminant);
        double intersection_t = (-quadratic_b - discriminant) / quadratic_a; // First intersection
        double axis_projection = axis_dot_origin_to_start + intersection_t * axis_dot_direction;

        double final_intersection_t = -1.0;
        vec3 surface_normal;
        point3 intersection_point;

        // Check intersection with the cylindrical body
        if (intersection_t > 0.0 && ray_t.contains(intersection_t) && axis_projection > 0.0 && axis_projection < height) {
            final_intersection_t = intersection_t;
            intersection_point = r.at(intersection_t);
            vec3 point_to_cylinder_start = intersection_point - base_center;
            double projection_scale = dot(point_to_cylinder_start, unit_cylinder_axis);
            point3 closest_point_on_axis = base_center + unit_cylinder_axis * projection_scale;
            surface_normal = unit_vector(intersection_point - closest_point_on_axis);
        }

        // If the first root is invalid, try the second root
        if (final_intersection_t < 0.0) {
            intersection_t = (-quadratic_b + discriminant) / quadratic_a; // Second intersection
            axis_projection = axis_dot_origin_to_start + intersection_t * axis_dot_direction;

            if (intersection_t > 0.0 && ray_t.contains(intersection_t) && axis_projection > 0.0 && axis_projection < height) {
                final_intersection_t = intersection_t;
                intersection_point = r.at(intersection_t);
                vec3 point_to_cylinder_start = intersection_point - base_center;
                double projection_scale = dot(point_to_cylinder_start, unit_cylinder_axis);
                point3 closest_point_on_axis = base_center + unit_cylinder_axis * projection_scale;
                surface_normal = unit_vector(intersection_point - closest_point_on_axis);
            }
        }

        if (capped) {
            double cap_intersection_t;

            // Lower cap (base)
            if (axis_dot_direction != 0.0) {
                cap_intersection_t = -axis_dot_origin_to_start / axis_dot_direction;
                if (ray_t.contains(cap_intersection_t) && cap_intersection_t > 0.0) {
                    point3 base_intersection = r.at(cap_intersection_t);
                    double distance_squared = (base_intersection - base_center).length_squared();
                    if (distance_squared <= radius_squared + epsilon &&
                        (final_intersection_t < 0.0 || cap_intersection_t < final_intersection_t)) {
                        final_intersection_t = cap_intersection_t;
                        intersection_point = base_intersection;
                        surface_normal = -unit_cylinder_axis;
                    }
                }

                // Upper cap (top)
                cap_intersection_t = (height - axis_dot_origin_to_start) / axis_dot_direction;
                if (ray_t.contains(cap_intersection_t) && cap_intersection_t > 0.0) {
                    point3 top_intersection = r.at(cap_intersection_t);
                    double distance_squared = (top_intersection - top_center).length_squared();
                    if (distance_squared <= radius_squared + epsilon &&
                        (final_intersection_t < 0.0 || cap_intersection_t < final_intersection_t)) {
                        final_intersection_t = cap_intersection_t;
                        intersection_point = top_intersection;
                        surface_normal = unit_cylinder_axis;
                    }
                }
            }
        }

        if (final_intersection_t < 0.0)
            return false;

        rec.t = final_intersection_t;
        rec.p = intersection_point;
        rec.set_face_normal(r, surface_normal);
        rec.material = &material;
        rec.hit_object = this;
        return true;
    }

    bool hit_all(const ray& r, interval ray_t, std::vector<hit_record>& recs) const override {
        vec3 ray_origin = r.origin();
        vec3 ray_direction = r.direction();
        vec3 origin_to_cylinder_start = ray_origin - base_center;
        const double epsilon = 1e-6;

        // Precompute dot products for the ray and cylinder axis
        double axis_dot_direction = dot(unit_cylinder_axis, ray_direction);
        double axis_dot_origin_to_start = dot(unit_cylinder_axis, origin_to_cylinder_start);

        // Solve the quadratic equation for intersection with the infinite cylinder
        double quadratic_a = 1.0 - axis_dot_direction * axis_dot_direction;
        double quadratic_b = dot(origin_to_cylinder_start, ray_direction) - axis_dot_origin_to_start * axis_dot_direction;
        double quadratic_c = dot(origin_to_cylinder_start, origin_to_cylinder_start) - axis_dot_origin_to_start * axis_dot_origin_to_start - radius_squared;

        double discriminant = quadratic_b * quadratic_b - quadratic_a * quadratic_c;
        if (discriminant < 0.0)
            return false;

        discriminant = std::sqrt(discriminant);
        double t1 = (-quadratic_b - discriminant) / quadratic_a; // First intersection
        double t2 = (-quadratic_b + discriminant) / quadratic_a; // Second intersection

        // Check intersections with the cylindrical body
        auto add_cylinder_hit = [&](double t) {
            if (t > 0.0 && ray_t.contains(t)) {
                double axis_projection = axis_dot_origin_to_start + t * axis_dot_direction;
                if (axis_projection > 0.0 && axis_projection < height) {
                    hit_record rec;
                    rec.t = t;
                    rec.p = r.at(t);
                    vec3 point_to_cylinder_start = rec.p - base_center;
                    double projection_scale = dot(point_to_cylinder_start, unit_cylinder_axis);
                    point3 closest_point_on_axis = base_center + unit_cylinder_axis * projection_scale;
                    rec.set_face_normal(r, unit_vector(rec.p - closest_point_on_axis));
                    rec.material = &material;
                    rec.hit_object = this;
                    recs.push_back(rec);
                }
            }
            };

        add_cylinder_hit(t1);
        add_cylinder_hit(t2);

        // Check intersections with the caps (if capped)
        if (capped) {
            auto add_cap_hit = [&](double t, const point3& cap_center, const vec3& normal) {
                if (t > 0.0 && ray_t.contains(t)) {
                    point3 intersection_point = r.at(t);
                    double distance_squared = (intersection_point - cap_center).length_squared();
                    if (distance_squared <= radius_squared + epsilon) {
                        hit_record rec;
                        rec.t = t;
                        rec.p = intersection_point;
                        rec.set_face_normal(r, normal);
                        rec.material = &material;
                        rec.hit_object = this;
                        recs.push_back(rec);
                    }
                }
                };

            // Lower cap (base)
            if (axis_dot_direction != 0.0) {
                double t_base = -axis_dot_origin_to_start / axis_dot_direction;
                add_cap_hit(t_base, base_center, -unit_cylinder_axis);
            }

            // Upper cap (top)
            if (axis_dot_direction != 0.0) {
                double t_top = (height - axis_dot_origin_to_start) / axis_dot_direction;
                add_cap_hit(t_top, top_center, unit_cylinder_axis);
            }
        }

        // Sort hits by t value
        std::sort(recs.begin(), recs.end(), [](const hit_record& a, const hit_record& b) {
            return a.t < b.t;
            });

        return !recs.empty();
    }

    bool IsPointInside(const point3& p) const {
        // 1) Project p onto the cylinder's axis
        double proj = dot(p - base_center, unit_cylinder_axis);

        // 2) Check if the projection is within the cylinder’s height
        if (proj < 0 || proj > height) {
            return false; // outside along the cylinder's axis
        }

        // 3) Find the closest point on the cylinder axis
        point3 closest_on_axis = base_center + proj * unit_cylinder_axis;

        // 4) Check the radial distance
        double dist_sq = (p - closest_on_axis).length_squared();
        return dist_sq <= radius_squared;
    }

    char test_bb(const BoundingBox& bb) const {
        const auto vertices = bb.getVertices();
        // Count how many corners are inside
        unsigned int inside_count = 0;
        for (auto& v : vertices) {
            if (IsPointInside(v)) {
                inside_count++;
            }
        }
        if (inside_count == 8) {
            return 'b'; // all corners inside
        }
        else if (inside_count == 0) {
            // Check the center of the box
            point3 c = bb.getCenter();
            if (IsPointInside(c)) {
                return 'g';
            }

            // Check the center of each face
            point3 dims = bb.getDimensions();
            vec3 half_dims = dims / 2.0; // Precompute half-dimensions for reuse
            point3 faceCenters[6] = {
                bb.vmin + vec3(half_dims.x(), half_dims.y(), 0),                 // "top" face center
                bb.vmin + vec3(half_dims.x(), half_dims.y(), dims.z()),          // "bottom" face center
                bb.vmin + vec3(half_dims.x(), 0, half_dims.z()),                 // front
                bb.vmin + vec3(half_dims.x(), dims.y(), half_dims.z()),          // back
                bb.vmin + vec3(0, half_dims.y(), half_dims.z()),                 // left
                bb.vmin + vec3(dims.x(), half_dims.y(), half_dims.z())           // right
            };

            for (auto& fc : faceCenters) {
                if (IsPointInside(fc)) {
                    return 'g';
                }
            }
            return 'w'; // No parts inside
        }
        else {
            // Some corners in, some out => partial
            return 'g';
        }
    }

    void transform(const Matrix4x4& matrix) override {
        // Transform the base and top center points
        base_center = matrix.transform_point(base_center);
        top_center = matrix.transform_point(top_center);

        // Recompute the cylinder axis
        update_constants();
    }

    BoundingBox bounding_box() const override {
        // Calculate the min and max points in the axis-aligned directions
        point3 min_point(
            std::min(base_center.x() - radius, top_center.x() - radius),
            std::min(base_center.y() - radius, top_center.y() - radius),
            std::min(base_center.z() - radius, top_center.z() - radius)
        );

        point3 max_point(
            std::max(base_center.x() + radius, top_center.x() + radius),
            std::max(base_center.y() + radius, top_center.y() + radius),
            std::max(base_center.z() + radius, top_center.z() + radius)
        );

        return BoundingBox(min_point, max_point);
    }

    std::string get_type_name() const override {
        return "Cylinder";
    }

private:
    void update_constants() {
        cylinder_axis = top_center - base_center;
        axis_length_squared = dot(cylinder_axis, cylinder_axis);
        unit_cylinder_axis = cylinder_axis / std::sqrt(axis_length_squared);
        reciprocal_axis_length_squared = 1.0 / axis_length_squared;
        radius_squared = radius * radius;
    }

    point3 base_center;
    point3 top_center;
    double radius;
    double height;
    bool capped;
    vec3 cylinder_axis;

    vec3 unit_cylinder_axis;
    double axis_length_squared;
    double reciprocal_axis_length_squared;
    double radius_squared;

    mat material;
};

#endif