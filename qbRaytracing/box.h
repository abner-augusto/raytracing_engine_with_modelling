#ifndef BOX_H
#define BOX_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"

class box : public hittable {
public:
    box(const point3& v_min, const point3& v_max, const mat& material)
        : min_corner(v_min), v_max(v_max), material(material) {
    }

    void set_min_corner(const point3& new_min_corner) {
        min_corner = new_min_corner;
    }

    void set_max_corner(const point3& new_max_corner) {
        v_max = new_max_corner;
    }

    void set_material(const mat& new_material) {
        material = new_material;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        double t_min = ray_t.min;
        double t_max = ray_t.max;

        // Iterate over x, y, z axes
        for (int i = 0; i < 3; ++i) {
            double inv_d = 1.0 / r.direction()[i];
            double t0 = (min_corner[i] - r.origin()[i]) * inv_d;
            double t1 = (v_max[i] - r.origin()[i]) * inv_d;

            if (inv_d < 0.0) {
                std::swap(t0, t1);
            }

            t_min = std::fmax(t0, t_min);
            t_max = std::fmin(t1, t_max);

            if (t_max <= t_min) {
                return false;
            }
        }

        rec.t = t_min;
        rec.p = r.at(t_min);

        // Determine which face was hit
        vec3 outward_normal;
        for (int i = 0; i < 3; ++i) {
            if (std::fabs(rec.p[i] - min_corner[i]) < 1e-4) {
                outward_normal[i] = -1.0;
            }
            else if (std::fabs(rec.p[i] - v_max[i]) < 1e-4) {
                outward_normal[i] = 1.0;
            }
            else {
                outward_normal[i] = 0.0;
            }
        }

        rec.set_face_normal(r, outward_normal);
        rec.material = &material;

        return true;
    }

private:
    point3 min_corner;
    point3 v_max;
    mat material;
};

#endif
