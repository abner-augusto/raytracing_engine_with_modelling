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
        const double epsilon = 1e-7;

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

    bool csg_intersect(const ray& r, interval ray_t,
        std::vector<CSGIntersection>& out_intersections) const override {
        out_intersections.clear();

        // Vector to store potential hits (t, normal)
        std::vector<std::pair<double, vec3>> hits;

        const vec3 ray_origin = r.origin();
        const vec3 ray_direction = r.direction();
        const vec3 origin_to_base = ray_origin - base_center;

        double axis_dot_dir = dot(unit_cylinder_axis, ray_direction);
        double axis_dot_org = dot(unit_cylinder_axis, origin_to_base);

        // Quadratic coefficients for the infinite cylinder
        double a = 1.0 - axis_dot_dir * axis_dot_dir;
        double b = dot(origin_to_base, ray_direction) - axis_dot_org * axis_dot_dir;
        double c = dot(origin_to_base, origin_to_base) - axis_dot_org * axis_dot_org - radius_squared;

        const double EPS = 1e-7;

        // Determine if the ray starts inside the cylinder
        bool ray_starts_inside = is_point_inside(ray_origin);

        // Solve for side intersections
        if (std::fabs(a) > EPS) {
            double discriminant = b * b - a * c;
            if (discriminant >= 0.0) {
                double sqrt_disc = std::sqrt(discriminant);
                double t1 = (-b - sqrt_disc) / a;
                double t2 = (-b + sqrt_disc) / a;

                // Check first intersection (t1)
                if (t1 >= ray_t.min && t1 <= ray_t.max) {
                    double proj = axis_dot_org + t1 * axis_dot_dir;
                    if (proj >= 0.0 && proj <= height) {
                        point3 p_hit = r.at(t1);
                        vec3 outward_normal = unit_vector(p_hit - (base_center + unit_cylinder_axis * proj));
                        hits.emplace_back(t1, outward_normal);
                    }
                }

                // Check second intersection (t2)
                if (t2 >= ray_t.min && t2 <= ray_t.max) {
                    double proj = axis_dot_org + t2 * axis_dot_dir;
                    if (proj >= 0.0 && proj <= height) {
                        point3 p_hit = r.at(t2);
                        vec3 outward_normal = unit_vector(p_hit - (base_center + unit_cylinder_axis * proj));
                        hits.emplace_back(t2, outward_normal);
                    }
                }
            }
        }

        // Check for cap intersections if the cylinder is capped
        if (capped) {
            // Bottom cap
            if (std::fabs(axis_dot_dir) > EPS) {
                double t_bottom = -axis_dot_org / axis_dot_dir;
                if (t_bottom >= ray_t.min && t_bottom <= ray_t.max) {
                    point3 p_hit = r.at(t_bottom);
                    double dist2 = (p_hit - base_center).length_squared();
                    if (dist2 <= radius_squared + EPS) {
                        vec3 outward_normal = -unit_cylinder_axis;
                        hits.emplace_back(t_bottom, outward_normal);
                    }
                }

                // Top cap
                double t_top = (height - axis_dot_org) / axis_dot_dir;
                if (t_top >= ray_t.min && t_top <= ray_t.max) {
                    point3 p_hit = r.at(t_top);
                    double dist2 = (p_hit - top_center).length_squared();
                    if (dist2 <= radius_squared + EPS) {
                        vec3 outward_normal = unit_cylinder_axis;
                        hits.emplace_back(t_top, outward_normal);
                    }
                }
            }
        }

        // Sort hits by ascending t
        std::sort(hits.begin(), hits.end(),
            [](const std::pair<double, vec3>& a, const std::pair<double, vec3>& b) {
                return a.first < b.first;
            });

        // Process each hit to assign is_entry flags and store in out_intersections
        for (const auto& [t_val, normal] : hits) {
            if (t_val < ray_t.min || t_val > ray_t.max)
                continue;

            point3 p_hit = r.at(t_val);
            bool is_entry;

            if (!ray_starts_inside) {
                // For rays starting outside, the first hit is an entry, the second is an exit
                is_entry = out_intersections.empty();
            }
            else {
                // For rays starting inside, the first hit is an exit, the second is an entry
                is_entry = !out_intersections.empty();
            }

            // Invert the normal if it's an exit
            vec3 adjusted_normal = normal;
            if (!is_entry) {
                adjusted_normal = -normal;
            }

            // Add the intersection with the possibly adjusted normal
            out_intersections.emplace_back(
                t_val,
                is_entry,
                this,             // 'obj' pointer = this cylinder
                adjusted_normal,  // Adjusted normal
                p_hit             // Intersection point
            );
        }

        // Sort final results by t (just to be sure)
        std::sort(out_intersections.begin(), out_intersections.end(),
            [](const CSGIntersection& a, const CSGIntersection& b) {
                return a.t < b.t;
            });

        return !out_intersections.empty();
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
        // Compute the maximum lateral extension along each coordinate direction.
        // We use std::max(0.0, …) to protect against any floating-point round-off issues.
        double extend_x = radius * std::sqrt(std::max(0.0, 1.0 - unit_cylinder_axis.x() * unit_cylinder_axis.x()));
        double extend_y = radius * std::sqrt(std::max(0.0, 1.0 - unit_cylinder_axis.y() * unit_cylinder_axis.y()));
        double extend_z = radius * std::sqrt(std::max(0.0, 1.0 - unit_cylinder_axis.z() * unit_cylinder_axis.z()));

        // For each axis, get the minimum and maximum of the base and top centers, then extend by the calculated offsets.
        double min_x = std::min(base_center.x(), top_center.x()) - extend_x;
        double max_x = std::max(base_center.x(), top_center.x()) + extend_x;
        double min_y = std::min(base_center.y(), top_center.y()) - extend_y;
        double max_y = std::max(base_center.y(), top_center.y()) + extend_y;
        double min_z = std::min(base_center.z(), top_center.z()) - extend_z;
        double max_z = std::max(base_center.z(), top_center.z()) + extend_z;

        return BoundingBox(point3(min_x, min_y, min_z), point3(max_x, max_y, max_z));
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