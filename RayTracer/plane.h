#ifndef PLANE_H
#define PLANE_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"

class plane : public hittable {
public:
    plane(const point3& point_on_plane, const vec3& normal_vector, const mat& material)
        : point(point_on_plane), normal(unit_vector(normal_vector)), material(material) {
    }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Compute the denominator of the intersection formula
        auto denom = dot(normal, r.direction());

        // Se o denominador for próximo de zero, o raio é paralelo ao plano
        if (fabs(denom) < 1e-6) {
            return false;
        }

        // Calcula o ponto de interseção
        auto t = dot(point - r.origin(), normal) / denom;

        // Verifica se t está dentro do intervalo válido
        if (!ray_t.surrounds(t)) {
            return false;
        }

        rec.t = t;
        rec.p = r.at(rec.t);
        rec.set_face_normal(r, normal);
        rec.material = &material;

        return true;
    }

private:
    point3 point;       // Um ponto no plano
    vec3 normal;        // O vetor normal do plano (deve ser normalizado)
    mat material;
};

#endif
