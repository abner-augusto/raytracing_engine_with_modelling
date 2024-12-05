#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"

class sphere : public hittable {
public:
    sphere(const point3& center, double radius, const mat& material)
        : center(center), radius(std::fmax(0, radius)), material(material) {
    }

    void set_center(const point3& new_center) {
        center = new_center;
    }

    void set_radius(double new_radius) {
        radius = std::fmax(0, new_radius);
    }

    void set_material(const mat& new_material) {
        material = new_material;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;

        auto discriminant = half_b * half_b - a * c;
        if (discriminant < 0)
            return false;

        auto sqrtd = std::sqrt(discriminant);

        // Encontra a raiz mais próxima que esteja dentro do intervalo aceitável.
        auto root = (-half_b - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (-half_b + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.material = &material;

        return true;
    }

private:
    point3 center;
    double radius;
    mat material;
};

#endif
