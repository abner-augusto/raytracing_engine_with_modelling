#ifndef CYLINDER_H
#define CYLINDER_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include "boundingbox.h"
#include "primitive.h"
#include <cmath>

class cylinder : public hittable, public Primitive {
public:
    cylinder(const point3& base_center, double height, double radius, const mat& material, bool capped = true)
        : a(base_center),
        radius(std::fmax(0, radius)),
        material(material),
        capped(capped),
        height(height)
    {
        b = a + vec3(0, height, 0);
        cylinder_axis = b - a;
        axis_length_squared = dot(cylinder_axis, cylinder_axis);
    }

    cylinder(const point3& base_center, const point3& top_center, double radius, const mat& material, bool capped = true)
        : a(base_center), b(top_center), radius(std::fmax(0, radius)), material(material), capped(capped)
    {
        cylinder_axis = b - a;
        axis_length_squared = dot(cylinder_axis, cylinder_axis);
        height = (b - a).length();
    }

    void set_base_center(const point3& new_base_center) {
        a = new_base_center;
        b = a + vec3(0, height, 0);
        cylinder_axis = b - a;
        axis_length_squared = dot(cylinder_axis, cylinder_axis);
    }

    void set_height(double new_height) {
        height = new_height;
        b = a + vec3(0, height, 0);
        cylinder_axis = b - a;
        axis_length_squared = dot(cylinder_axis, cylinder_axis);
    }

    void set_top_center(const point3& new_top_center) {
        b = new_top_center;
        cylinder_axis = b - a;
        axis_length_squared = dot(cylinder_axis, cylinder_axis);
        height = (b - a).length();
    }

    void set_radius(double new_radius) {
        radius = std::fmax(0, new_radius);
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
        vec3 origin_to_cylinder_start = ray_origin - a;


        double axis_dot_direction = dot(cylinder_axis, ray_direction);
        double axis_dot_origin_to_start = dot(cylinder_axis, origin_to_cylinder_start);

        double quadratic_a = axis_length_squared - axis_dot_direction * axis_dot_direction;
        double quadratic_b = axis_length_squared * dot(origin_to_cylinder_start, ray_direction) - axis_dot_origin_to_start * axis_dot_direction;
        double quadratic_c = axis_length_squared * dot(origin_to_cylinder_start, origin_to_cylinder_start) - axis_dot_origin_to_start * axis_dot_origin_to_start - radius * radius * axis_length_squared;

        double discriminant = quadratic_b * quadratic_b - quadratic_a * quadratic_c;
        if (discriminant < 0.0)
            return false;

        discriminant = std::sqrt(discriminant);
        double intersection_t = (-quadratic_b - discriminant) / quadratic_a; // primeira interseção
        double axis_projection = axis_dot_origin_to_start + intersection_t * axis_dot_direction;

        double final_intersection_t = -1.0;
        vec3 surface_normal;
        point3 intersection_point;

        // Verifica interseção com o corpo do cilindro
        if (intersection_t > 0.0 && ray_t.contains(intersection_t) && axis_projection > 0.0 && axis_projection < axis_length_squared) {
            final_intersection_t = intersection_t;
            intersection_point = r.at(intersection_t);
            vec3 point_to_cylinder_start = intersection_point - a;
            double projection_scale = axis_projection / axis_length_squared;
            surface_normal = (point_to_cylinder_start - cylinder_axis * projection_scale) / radius;
        }

        // Caso a primeira raiz não seja válida, tenta a segunda raiz
        if (final_intersection_t < 0.0) {
            intersection_t = (-quadratic_b + discriminant) / quadratic_a;
            axis_projection = axis_dot_origin_to_start + intersection_t * axis_dot_direction;
            if (intersection_t > 0.0 && ray_t.contains(intersection_t) && axis_projection > 0.0 && axis_projection < axis_length_squared) {
                final_intersection_t = intersection_t;
                intersection_point = r.at(intersection_t);
                vec3 point_to_cylinder_start = intersection_point - a;
                double projection_scale = axis_projection / axis_length_squared;
                surface_normal = (point_to_cylinder_start - cylinder_axis * projection_scale) / radius;
            }
        }

        // Verifica tampas do cilindro se não encontrou no corpo ou se elas estão mais próximas
        if (capped) {
            double cap_intersection_t;

            if (capped && axis_dot_direction != 0.0) {
                cap_intersection_t = (0.0 - axis_dot_origin_to_start) / axis_dot_direction;
                if (ray_t.contains(cap_intersection_t) && cap_intersection_t > 0.0) {
                    double validity_check = (quadratic_b + quadratic_a * cap_intersection_t);
                    point3 base_intersection = r.at(cap_intersection_t);
                    double distance_squared = (base_intersection - a).length_squared();
                    if (distance_squared <= radius * radius && (final_intersection_t < 0.0 || cap_intersection_t < final_intersection_t)) {
                        final_intersection_t = cap_intersection_t;
                        intersection_point = base_intersection;
                        surface_normal = -(cylinder_axis / std::sqrt(axis_length_squared)); // normal da base inferior
                    }
                }

                // Base superior (axis_projection = axis_length_squared)
                cap_intersection_t = (axis_length_squared - axis_dot_origin_to_start) / axis_dot_direction;
                if (ray_t.contains(cap_intersection_t) && cap_intersection_t > 0.0) {
                    point3 top_intersection = r.at(cap_intersection_t);
                    double distance_squared = (top_intersection - b).length_squared();
                    if (distance_squared <= radius * radius && (final_intersection_t < 0.0 || cap_intersection_t < final_intersection_t)) {
                        final_intersection_t = cap_intersection_t;
                        intersection_point = top_intersection;
                        surface_normal = cylinder_axis / std::sqrt(axis_length_squared); // normal da base superior
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
        return true;
    }

    bool IsPointInside(const point3& p) const {
        // 1) Project p onto the cylinder's axis
        vec3 axis = unit_vector(cylinder_axis);
        double proj = dot(p - a, axis);

        // 2) Check if the projection is within the cylinder’s height
        //    (assuming a is the base and b = a + axis * height is the top)
        if (proj < 0 || proj > height) {
            return false; // outside along the cylinder's axis
        }

        // 3) Find the closest point on the cylinder axis
        point3 closest_on_axis = a + proj * axis;

        // 4) Check the radial distance
        double dist_sq = (p - closest_on_axis).length_squared();
        double r_sq = radius * radius;
        return dist_sq <= r_sq;
    }

    char test_bb(const BoundingBox& bb) const {
        const auto vertices = bb.Vertices();

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
            // 1) Check the center of the box
            point3 c = bb.Center();
            if (IsPointInside(c)) {
                return 'g';
            }

            // 2) Optionally check the center of each face
            point3 faceCenters[6] = {
                bb.vmin + vec3(bb.width / 2, bb.width / 2, 0),          // "top" face center
                bb.vmin + vec3(bb.width / 2, bb.width / 2, bb.width),   // "bottom" face center
                bb.vmin + vec3(bb.width / 2, 0, bb.width / 2),          // front 
                bb.vmin + vec3(bb.width / 2, bb.width, bb.width / 2),   // back
                bb.vmin + vec3(0, bb.width / 2, bb.width / 2),          // left
                bb.vmin + vec3(bb.width, bb.width / 2, bb.width / 2)    // right
            };

            for (auto& fc : faceCenters) {
                if (IsPointInside(fc)) {
                    return 'g';
                }
            }

            return 'w';
        }
        else {
            // Some corners in, some out => partial
            return 'g';
        }
    }

private:
    point3 a; // base_center
    point3 b; // top_center
    double radius;
    mat material;
    bool capped;
    vec3 cylinder_axis;
    double axis_length_squared;
    double height;
};

#endif