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

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 ro = r.origin();
        vec3 rd = r.direction();
        vec3 co = ro - top_vertex; // vetores do apex para a origem do raio

        double vv = dot(axis_direction, axis_direction); // deve ser 1, já que axis_direction é unitário
        double dv = dot(rd, axis_direction);
        double cv = dot(co, axis_direction);

        double cosa2 = cos_angle_sq;
        double rdco = dot(rd, co);
        double coco = dot(co, co);

        double a = dv * dv - cosa2;
        double b = 2.0 * (dv * cv - rdco * cosa2);
        double c = cv * cv - coco * cosa2;

        double discriminant = b * b - 4.0 * a * c;
        if (discriminant < 0.0) return false;

        double sqrt_disc = std::sqrt(discriminant);

        double t1 = (-b - sqrt_disc) / (2.0 * a);
        double t2 = (-b + sqrt_disc) / (2.0 * a);

        bool hit_found = false;
        double t = 0.0;
        vec3 cp; // cp = ponto de interseção - apex

        // Verifica t1
        if (t1 > 0.0 && ray_t.contains(t1)) {
            cp = (ro + t1 * rd) - top_vertex;
            double h_val = dot(cp, axis_direction);
            if (h_val >= 0.0 && h_val <= height) {
                hit_found = true;
                t = t1;
            }
        }

        // Verifica t2, se t1 não foi válido ou se t2 é menor
        if (t2 > 0.0 && ray_t.contains(t2)) {
            vec3 cp2 = (ro + t2 * rd) - top_vertex;
            double h_val2 = dot(cp2, axis_direction);
            if (h_val2 >= 0.0 && h_val2 <= height && (!hit_found || t2 < t)) {
                hit_found = true;
                t = t2;
                cp = cp2;
            }
        }

        if (!hit_found) return false;

        double h_val = dot(cp, axis_direction);
        vec3 normal = unit_vector(cp - axis_direction * h_val);

        // Verifica a base se for tampado.
        if (capped) {
            // Verifica interseção com a base (plano da base)
            double denom = dot(rd, axis_direction);
            if (std::fabs(denom) > 1e-8) {
                double t_base = dot(base_center - ro, axis_direction) / denom;
                if (ray_t.contains(t_base) && t_base > 0.0) {
                    point3 p_base = r.at(t_base);
                    if ((p_base - base_center).length_squared() <= radius * radius) {
                        // Se a base for mais próxima, use ela
                        if (!hit_found || t_base < t) {
                            t = t_base;
                            cp = (p_base - top_vertex); // só para coerência, se precisar
                            normal = -axis_direction;
                            hit_found = true;
                        }
                    }
                }
            }
        }

        if (!hit_found) return false;

        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, normal);
        rec.material = &material;
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
