#pragma once

#include <vector>
#include <memory>
#include <utility>
#include "hittable_manager.h"
#include "material.h"

//#include "sphere.h"
//#include "plane.h"
//#include "cylinder.h"
//#include "cone.h"
//#include "box.h"
//#include "torus.h"

class SceneBuilder {
public:
    static void buildAtividade6Scene(
        SceneManager& world,
        const mat& grass_material,
        const mat& orange_material,
        const mat& white_material,
        const mat& brown_material,
        const mat& green_material,
        const mat& red_material,
        const mat& brick_material,
        const mat& wood_material
    );

private:
    static void applySceneTransformations(SceneManager& world);
};