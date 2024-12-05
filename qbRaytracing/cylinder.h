#ifndef CYLINDER_H
#define CYLINDER_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include <algorithm>

class cylinder : public hittable {
public:
    cylinder(const point3& base_center, const point3& top_center, double radius, const mat& material, bool capped = true)
        : base_center(base_center), top_center(top_center), radius(std::fmax(0, radius)), material(material), capped(capped) {
        update_axis();
    }

    void set_base_center(const point3& new_base_center) {
        base_center = new_base_center;
        update_axis();
    }

    void set_top_center(const point3& new_top_center) {
        top_center = new_top_center;
        update_axis();
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
        // Vetores auxiliares
        vec3 d = r.direction();
        vec3 m = r.origin() - base_center;
        vec3 A = axis_direction; // Vetor unitário ao longo do eixo

        double d_dot_A = dot(d, A);
        double m_dot_A = dot(m, A);

        vec3 Dd = d - d_dot_A * A;
        vec3 Dm = m - m_dot_A * A;

        double a = Dd.length_squared();
        double b = 2 * dot(Dd, Dm);
        double c = Dm.length_squared() - radius * radius;

        double discriminant = b * b - 4 * a * c;
        if (discriminant < 0)
            return false;

        double sqrt_discriminant = std::sqrt(discriminant);

        // Inicializa o registro de colisão com t máximo possível
        rec.t = ray_t.max;
        bool hit_anything = false;

        // Verifica as duas raízes da equação quadrática para a superfície lateral
        for (int i = 0; i < 2; ++i) {
            double t = (-b + (i == 0 ? -sqrt_discriminant : sqrt_discriminant)) / (2 * a);
            if (!ray_t.surrounds(t))
                continue;

            double k = m_dot_A + t * d_dot_A;
            if (k < 0 || k > height)
                continue;

            if (t < rec.t) {
                rec.t = t;
                rec.p = r.at(t);

                vec3 normal = (Dm + t * Dd) - k * A;
                rec.set_face_normal(r, unit_vector(normal));
                rec.material = &material;
                hit_anything = true;
            }
        }

        // Se o cilindro for tampado, verifique colisões com as bases
        if (capped) {
            const double epsilon = 1e-8;

            // Verifica colisão com a base inferior do cilindro
            double denom = -d_dot_A;
            if (std::fabs(denom) > epsilon) {
                double t = -m_dot_A / denom;
                if (ray_t.contains(t)) {
                    point3 p = r.at(t);
                    if ((p - base_center).length_squared() <= radius * radius) {
                        if (t < rec.t) {
                            rec.t = t;
                            rec.p = p;
                            rec.set_face_normal(r, -A);
                            rec.material = &material;
                            hit_anything = true;
                        }
                    }
                }
            }

            // Verifica colisão com a base superior do cilindro
            denom = d_dot_A;
            if (std::fabs(denom) > epsilon) {
                double t = (height - m_dot_A) / denom;
                if (ray_t.contains(t)) {
                    point3 p = r.at(t);
                    if ((p - top_center).length_squared() <= radius * radius) {
                        if (t < rec.t) {
                            rec.t = t;
                            rec.p = p;
                            rec.set_face_normal(r, A);
                            rec.material = &material;
                            hit_anything = true;
                        }
                    }
                }
            }
        }

        return hit_anything;
    }

private:
    point3 base_center;
    point3 top_center;
    vec3 axis;
    vec3 axis_direction;
    double height;
    double radius;
    mat material;
    bool capped;

    void update_axis() {
        axis = top_center - base_center;
        height = axis.length();
        axis_direction = axis / height;
    }
};

#endif
