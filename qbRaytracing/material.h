#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"

struct mat {
    color diffuse_color;  // Cor difusa do material
    double k_diffuse;     // Coeficiente de reflex�o difusa
    double k_specular;    // Coeficiente de reflex�o especular
    double shininess;     // Valor de brilho do material

    // Construtor com valores padr�o para um material fosco (matte)
    mat(const color& diffuse, double diffuseCoeff = 0.8, double specularCoeff = 0.3, double shine = 10.0)
        : diffuse_color(diffuse), k_diffuse(diffuseCoeff), k_specular(specularCoeff), shininess(shine) {
    }
};

#endif
