//#include "scene_builder.h"
//
//void SceneBuilder::buildAtividade6Scene(
//    SceneManager& world,
//    const mat& grass_material,
//    const mat& orange_material,
//    const mat& white_material,
//    const mat& brown_material,
//    const mat& green_material,
//    const mat& red_material,
//    const mat& brick_material,
//    const mat& wood_material
//) {
//    std::vector<std::pair<ObjectID, std::shared_ptr<hittable>>> Atividade6 = {
//        {1, make_shared<plane>(point3(0, 0, 0), vec3(0, 1, 0), grass_material, 0.5)},
//        // Mesa
//        {2, make_shared<box>(point3(0.0, 0.95, 0.0), 2.5, 0.05, 1.5, orange_material)},
//        {3, make_shared<box>(point3(0.0, 0.0, 0.0), 0.05, 0.95, 1.5, white_material)},
//        {4, make_shared<box>(point3(2.45, 0.0, 0.0), 0.05, 0.95, 1.5, white_material)},
//        // Arvore de Natal
//        {5, make_shared<cylinder>(point3(0, 0.0, 0.0), point3(0, 0.09, 0), 0.3, brown_material)},
//        {6, make_shared<cylinder>(point3(0, 0.09, 0.0), point3(0, 0.49, 0), 0.06, brown_material)},
//        {7, make_shared<cone>(point3(0, 0.49, 0.0), point3(0, 1.99, 0), 0.60, green_material)},
//        {8, make_shared<sphere>(point3(0, 2.0, 0.0), 0.045, red_material)},
//        // Portico 1
//        {9, make_shared<box>(point3(0, 0.0, 0.0), 0.5, 5.0, 0.3, white_material)},
//        {10, make_shared<box>(point3(0.0, 0.0, 0.0), 0.5, 5.0, 0.3, white_material)},
//        {11, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, white_material)},
//        {12, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, white_material)},
//        // Portico 2
//        {13, make_shared<box>(point3(0, 0.0, 0.0), 0.5, 5.0, 0.3, white_material)},
//        {14, make_shared<box>(point3(0.0, 0.0, 0.0), 0.5, 5.0, 0.3, white_material)},
//        {15, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, white_material)},
//        {16, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, white_material)},
//        // Telhado
//        {17, make_shared<box>(point3(0.0, 0.0, 0.0), 1.0, 0.1, 1.0, red_material)},
//        {18, make_shared<box>(point3(0.0, 0.0, 0.0), 1.0, 0.1, 1.0, red_material)},
//        // Paredes
//        {19, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, brick_material, 1.5)},
//        {20, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, brick_material, 1.5)},
//        {21, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, brick_material, 1.5)},
//        // Piso
//        {22, make_shared<box>(point3(0.0, 0.0, 0.0), 6.0, 0.1, 10.0, wood_material, 3)},
//    };
//
//    for (const auto& [id, obj] : Atividade6) {
//        world.add(obj, id);
//    }
//
//    applySceneTransformations(world);
//}
//
//void SceneBuilder::applySceneTransformations(SceneManager& world) {
//    point3 viga_vmin = point3(0.5, 0.0, 0.0);
//    point3 table_center = point3(1.25, 0.975, 0.75);
//
//    Matrix4x4 movetable;
//    movetable = movetable.translation(vec3(-1.25, 0, -5.75));
//    Matrix4x4 movetable_origin;
//    movetable_origin = movetable_origin.translation(-table_center);
//    Matrix4x4 movetable_back;
//    movetable_back = movetable_back.translation(table_center);
//
//    Matrix4x4 movetree;
//    movetree = movetree.translation(vec3(0, 1.0, -5));
//
//    Matrix4x4 movewall;
//    movewall = movewall.translation(vec3(3, 0, 0));
//
//    Matrix4x4 movewall2;
//    movewall2 = movewall2.translation(vec3(3, 0, -10));
//
//    Matrix4x4 movefloor;
//    movefloor = movefloor.translation(vec3(-3.0, 0.0, -10));
//
//    Matrix4x4 shear;
//    shear = shear.shearing(0.0, 0.75, 0.0);
//
//    Matrix4x4 viga_scale;
//    viga_scale = viga_scale.scaling(6.0, 1.0, 0.6);
//
//    Matrix4x4 telhado_scale;
//    telhado_scale = telhado_scale.scaling(4.5, 1.0, -9.7);
//
//    Matrix4x4 parede_scale;
//    parede_scale = parede_scale.scaling(0.2, 4.5, -10.0);
//
//    Matrix4x4 parede_scale2;
//    parede_scale2 = parede_scale2.scaling(0.2, 4.5, -6.0);
//
//    Matrix4x4 move;
//    move = move.translation(-viga_vmin);
//
//    Matrix4x4 moveup;
//    moveup = moveup.translation(vec3(-3.5, 4.5, 0));
//
//    Matrix4x4 movefar;
//    movefar = movefar.translation(vec3(0, 0, -10));
//
//    Matrix4x4 pilar_move;
//    pilar_move = pilar_move.translation(vec3(-3.5, 0.0, 0));
//
//    Matrix4x4 moveback;
//    moveback = moveback.translation(viga_vmin);
//
//    Matrix4x4 rotate;
//    rotate = rotate.rotation(37, 'Z');
//
//    Matrix4x4 parede_rotate;
//    parede_rotate = parede_rotate.rotation(90, 'y');
//
//    Matrix4x4 portico_mirror;
//    portico_mirror = portico_mirror.mirror('y');
//
//    Matrix4x4 mesa_transform = movetable_back * movetable * parede_rotate * movetable_origin;
//    Matrix4x4 viga_transform = moveback * moveup * shear * viga_scale * move;
//    Matrix4x4 telhado_transform = moveup * rotate * telhado_scale;
//    Matrix4x4 telhado_transform2 = portico_mirror * moveup * rotate * telhado_scale;
//    Matrix4x4 parede_transform = movewall * parede_scale;
//    Matrix4x4 parede_transform2 = movewall2 * parede_rotate * parede_scale2;
//
//    // Mesa
//    world.transform_range(2, 4, mesa_transform);
//    // Arvore de Natal
//    world.transform_range(5, 8, movetree);
//    // Portico
//    world.transform_range(9, 10, pilar_move);
//    world.transform_range(11, 12, viga_transform);
//    world.transform_object(10, portico_mirror);
//    world.transform_object(12, portico_mirror);
//
//    world.transform_range(13, 14, pilar_move);
//    world.transform_range(15, 16, viga_transform);
//    world.transform_object(14, portico_mirror);
//    world.transform_object(16, portico_mirror);
//
//    world.transform_range(13, 16, movefar);
//    // Telhado
//    world.transform_object(17, telhado_transform);
//    world.transform_object(18, telhado_transform2);
//    // Parede
//    world.transform_range(19, 20, parede_transform);
//    world.transform_object(20, portico_mirror);
//    world.transform_object(21, parede_transform2);
//    // Piso
//    world.transform_object(22, movefloor);
//}