#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"

class hit_record {
public:
    point3 p; // Ponto da colisão
    vec3 normal; // Normal no ponto de colisão
    double t = 0.0; // Valor da equação da reta
    bool front_face = true; // Para indicar se a colisão foi externo ou interno
    const mat* material = nullptr; // Ponteiro para o material do objeto colidido
    double u = 0.0; // Coordenada UV horizontal
    double v = 0.0; // Coordenada UV vertical

    hit_record() = default;

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    // Transform the object using a 4x4 matrix
    virtual void transform(const Matrix4x4& transform) = 0;

};

#endif
