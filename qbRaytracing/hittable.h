#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"

class hit_record {
public:
    point3 p;                 // Ponto da colis�o
    vec3 normal;              // Normal no ponto de colis�o
    double t;                 // Valor do par�metro t da equa��o da reta
    bool front_face;          // Para indicar se a colis�o foi do lado externo ou interno
    const mat* material; // Ponteiro para o material do objeto colidido

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        // Define o vetor normal no registro de colis�o.
        // OBS: o par�metro `outward_normal` � assumido como tendo comprimento unit�rio.

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};

#endif
