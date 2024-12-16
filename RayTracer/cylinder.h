#ifndef CYLINDER_H
#define CYLINDER_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include <cmath>

class cylinder : public hittable {
public:
    cylinder(const point3& base_center, const point3& top_center, double radius, const mat& material, bool capped = true)
        : a(base_center), b(top_center), ra(std::fmax(0, radius)), material(material), capped(capped)
    {
    }

    void set_base_center(const point3& new_base_center) {
        a = new_base_center;
    }

    void set_top_center(const point3& new_top_center) {
        b = new_top_center;
    }

    void set_radius(double new_radius) {
        ra = std::fmax(0, new_radius);
    }

    void set_material(const mat& new_material) {
        material = new_material;
    }

    void set_capped(bool new_capped) {
        capped = new_capped;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Baseado na lógica do shader:
        vec3 ro = r.origin();
        vec3 rd = r.direction();
        vec3 ba = b - a;
        vec3 oc = ro - a;

        double baba = dot(ba, ba);          // |ba|^2
        double bard = dot(ba, rd);
        double baoc = dot(ba, oc);

        double k2 = baba - bard * bard;
        double k1 = baba * dot(oc, rd) - baoc * bard;
        double k0 = baba * dot(oc, oc) - baoc * baoc - ra * ra * baba;

        double h = k1 * k1 - k2 * k0;
        if (h < 0.0)
            return false;

        h = std::sqrt(h);
        double t = (-k1 - h) / k2; // primeira interseção
        double y = baoc + t * bard;

        double final_t = -1.0;
        vec3 normal;
        point3 p;

        // Tenta corpo
        if (t > 0.0 && ray_t.contains(t) && y > 0.0 && y < baba) {
            final_t = t;
            p = r.at(t);
            // normal do corpo:
            vec3 pa = p - a;
            // projeção de pa em ba:
            double h_val = y / baba;
            normal = (pa - ba * h_val) / ra;
        }

        // Se não pegou a primeira raiz, tenta a segunda
        if (final_t < 0.0) {
            t = (-k1 + h) / k2;
            y = baoc + t * bard;
            if (t > 0.0 && ray_t.contains(t) && y > 0.0 && y < baba) {
                final_t = t;
                p = r.at(t);
                vec3 pa = p - a;
                double h_val = y / baba;
                normal = (pa - ba * h_val) / ra;
            }
        }

        // Tenta tampas se não achou corpo ou se as tampas podem ser mais próximas
        if (capped) {
            double t_cap;

            if (capped && bard != 0.0) {
                t_cap = (0.0 - baoc) / bard;
                if (ray_t.contains(t_cap) && t_cap > 0.0) {
                    double check_val = (k1 + k2 * t_cap);
                    // Se o |(k1 + k2*t)|<h, significa que o raio atinge dentro do raio do cilindro
                    // Porém, no shader eles usam outra condição, aqui podemos simplesmente verificar o raio da base:
                    point3 p_base = r.at(t_cap);
                    double dist_sq = (p_base - a).length_squared();
                    if (dist_sq <= ra * ra && (final_t < 0.0 || t_cap < final_t)) {
                        final_t = t_cap;
                        p = p_base;
                        normal = -(ba / std::sqrt(baba)); // normal da base inferior
                    }
                }

                // Base superior (y=baba)
                t_cap = (baba - baoc) / bard;
                if (ray_t.contains(t_cap) && t_cap > 0.0) {
                    point3 p_top = r.at(t_cap);
                    double dist_sq = (p_top - b).length_squared();
                    if (dist_sq <= ra * ra && (final_t < 0.0 || t_cap < final_t)) {
                        final_t = t_cap;
                        p = p_top;
                        normal = ba / std::sqrt(baba); // normal da base superior
                    }
                }
            }
        }

        if (final_t < 0.0)
            return false;

        rec.t = final_t;
        rec.p = p;
        rec.set_face_normal(r, normal);
        rec.material = &material;
        return true;
    }

private:
    point3 a; // base_center
    point3 b; // top_center
    double ra;
    mat material;
    bool capped;
};

#endif
