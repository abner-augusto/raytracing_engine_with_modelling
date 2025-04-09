#pragma once

#include "scene.h"
#include "material.h"

class SceneBuilder {
public:
    SceneBuilder();
    ~SceneBuilder();

    void buildAtividade6Scene(SceneManager& world);
    void buildSonicScene(SceneManager& world);

private:
    void applyAtividade6Transformations(SceneManager& world);

    // Cores
    color black;
    color white;
    color red;
    color orange;
    color green;
    color blue;
    color cyan;
    color brown;
    color yellow;

    // Texturas
    image_texture* wood_texture;
    image_texture* grass_texture;
    image_texture* brick_texture;

    // Texturas Procedurais
    checker_texture checker;
    checker_texture ground;

    // Materiais
    mat grass_material;
    mat orange_material;
    mat white_material;
    mat brown_material;
    mat green_material;
    mat red_material;
    mat brick_material;
    mat wood_material;
    mat checker_material;
    mat ground_material;
    mat yellow_material;
};
