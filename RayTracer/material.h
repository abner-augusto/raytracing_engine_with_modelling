#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "texture.h"

class mat {
public:
    // Tipo de mapeamento: cor sólida ou textura
    enum class Type { SolidColor, Texture };

    // Atributos do material
    Type type;                    // Tipo do material (cor sólida ou textura)
    color diffuse_color;          // Cor difusa (para SolidColor)
    const texture* texture_map;   // Ponteiro para textura (para Texture)
    double k_diffuse;             // Coeficiente de reflexão difusa
    double k_specular;            // Coeficiente de reflexão especular
    double shininess;             // Valor de brilho do material
    double reflection;            // Coeficiente de reflexão

    // Construtor para cor sólida
    mat(const color& diffuse = color(1, 1, 1), double diffuseCoeff = 0.8,
        double specularCoeff = 0.3, double shine = 10.0, double reflectCoeff = 0.0)
        : type(Type::SolidColor), diffuse_color(diffuse), texture_map(nullptr),
        k_diffuse(diffuseCoeff), k_specular(specularCoeff),
        shininess(shine), reflection(reflectCoeff) {
    }

    // Construtor para textura
    mat(const texture* tex, double diffuseCoeff = 0.8, double specularCoeff = 0.3,
        double shine = 10.0, double reflectCoeff = 0.0)
        : type(Type::Texture), diffuse_color(color(1, 1, 1)), texture_map(tex),
        k_diffuse(diffuseCoeff), k_specular(specularCoeff),
        shininess(shine), reflection(reflectCoeff) {
    }

    // Função para obter a cor do material, levando em conta a textura
    color get_color(double u, double v) const {
        if (type == Type::Texture && texture_map) {
            return texture_map->value(u, v); // Usa a textura
        }
        return diffuse_color; // Retorna cor sólida
    }
};

#endif
