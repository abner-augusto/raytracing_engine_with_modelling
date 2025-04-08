#ifndef MATERIAL_H
#define MATERIAL_H
#include "color.h"
#include "texture.h"

class mat {
public:
    // Material type enum
    enum class Type { SolidColor, Texture };

    // Material attributes
    Type type;                    // Material type (solid color or texture)
    color diffuse_color;          // Diffuse color (for SolidColor or fallback)
    const texture* texture_map;   // Pointer to texture (nullable)
    double k_diffuse;             // Diffuse reflection coefficient
    double k_specular;            // Specular reflection coefficient
    double shininess;             // Material shininess
    double reflection;            // Reflection coefficient

    // Constructor for solid color material
    mat(const color& diffuse = color(1, 1, 1), double diffuseCoeff = 0.8,
        double specularCoeff = 0.3, double shine = 10.0, double reflectCoeff = 0.0)
        : type(Type::SolidColor)
        , diffuse_color(diffuse)
        , texture_map(nullptr)
        , k_diffuse(diffuseCoeff)
        , k_specular(specularCoeff)
        , shininess(shine)
        , reflection(reflectCoeff) {
    }

    // Constructor for textured material (works for both image and checker textures)
    mat(const texture* tex, double diffuseCoeff = 0.8, double specularCoeff = 0.3,
        double shine = 10.0, double reflectCoeff = 0.0)
        : type(tex ? Type::Texture : Type::SolidColor)
        , diffuse_color(color(1, 1, 1))  // Default white if texture is used
        , texture_map(tex)
        , k_diffuse(diffuseCoeff)
        , k_specular(specularCoeff)
        , shininess(shine)
        , reflection(reflectCoeff) {
    }

    // Get material color at UV coordinates
    color get_color(double u, double v) const {
        if (type == Type::Texture && texture_map) {
            // All texture types (image or checker) use the same interface
            return texture_map->value(u, v);
        }
        return diffuse_color;
    }

    // Check if material has a valid texture
    bool has_texture() const {
        return type == Type::Texture && texture_map != nullptr;
    }
};

#endif