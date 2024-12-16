#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"

struct mat {
    color diffuse_color;  // Cor difusa do material
    double k_diffuse;     // Coeficiente de reflex�o difusa
    double k_specular;    // Coeficiente de reflex�o especular
    double shininess;     // Valor de brilho do material
    double reflection;    // Coeficiente de reflex�o

    // Construtor com valores padrão para um material fosco branco (matte)
    mat(const color& diffuse = color(1, 1, 1), double diffuseCoeff = 0.8,
        double specularCoeff = 0.3, double shine = 10.0, double reflectCoeff = 0.0)
        : diffuse_color(diffuse), k_diffuse(diffuseCoeff),
        k_specular(specularCoeff), shininess(shine),
        reflection(reflectCoeff) {
    }
};

#endif
