#include "raytracer.h"
#include "scene_builder.h"
#include "plane.h"
#include "box.h"
#include "cylinder.h"
#include "cone.h"
#include "sphere.h"
#include "torus.h"
#include "mesh.h"
#include "asset_path.h"

SceneBuilder::SceneBuilder()
    : black(0.0, 0.0, 0.0),
    white(1.0, 1.0, 1.0),
    red(1.0, 0.0, 0.0),
    orange(1.0, 0.5, 0.0),
    green(0.0, 1.0, 0.0),
    blue(0.0, 0.0, 1.0),
    cyan(0.0, 1.0, 0.9),
    brown(0.69, 0.49, 0.38),
    yellow(1, 1, 0),
    wood_texture(new image_texture(AssetPath::Resolve("textures/wood_floor.jpg"))),
    grass_texture(new image_texture(AssetPath::Resolve("textures/grass.jpg"))),
    brick_texture(new image_texture(AssetPath::Resolve("textures/brick.jpg"))),
    checker(black, white, 15),
    ground(color(0.43, 0.14, 0), color(0.86, 0.43, 0), 20),
    grass_material(grass_texture),
    orange_material(orange),
    white_material(white),
    brown_material(brown, 0.3, 0.3, 2.0),
    green_material(green),
    red_material(red, 0.8, 1.0, 150.0),
    brick_material(brick_texture),
    wood_material(wood_texture),
    checker_material(&checker, 0.8, 1.0, 100.0, 0.25),
    ground_material(&ground, 0.8, 1.0, 100.0, 0.25),
    yellow_material(yellow, 1.0, 1.0, 1000)
{
}

SceneBuilder::~SceneBuilder() {
    delete wood_texture;
    delete grass_texture;
    delete brick_texture;
}


void SceneBuilder::buildAtividade6Scene(SceneManager& world) {
    std::vector<std::pair<ObjectID, std::shared_ptr<hittable>>> Atividade6 = {
        {1, make_shared<plane>(point3(0, 0, 0), vec3(0, 1, 0), grass_material, 0.5)},
        // Mesa
        {2, make_shared<box>(point3(0.0, 0.95, 0.0), 2.5, 0.05, 1.5, orange_material)},
        {3, make_shared<box>(point3(0.0, 0.0, 0.0), 0.05, 0.95, 1.5, white_material)},
        {4, make_shared<box>(point3(2.45, 0.0, 0.0), 0.05, 0.95, 1.5, white_material)},
        // Arvore de Natal
        {5, make_shared<cylinder>(point3(0, 0.0, 0.0), point3(0, 0.09, 0), 0.3, brown_material)},
        {6, make_shared<cylinder>(point3(0, 0.09, 0.0), point3(0, 0.49, 0), 0.06, brown_material)},
        {7, make_shared<cone>(point3(0, 0.49, 0.0), point3(0, 1.99, 0), 0.60, green_material)},
        {8, make_shared<sphere>(point3(0, 2.0, 0.0), 0.045, red_material)},
        // Portico 1
        {9, make_shared<box>(point3(0, 0.0, 0.0), 0.5, 5.0, 0.3, white_material)},
        {10, make_shared<box>(point3(0.0, 0.0, 0.0), 0.5, 5.0, 0.3, white_material)},
        {11, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, white_material)},
        {12, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, white_material)},
        // Portico 2
        {13, make_shared<box>(point3(0, 0.0, 0.0), 0.5, 5.0, 0.3, white_material)},
        {14, make_shared<box>(point3(0.0, 0.0, 0.0), 0.5, 5.0, 0.3, white_material)},
        {15, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, white_material)},
        {16, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, white_material)},
        // Telhado
        {17, make_shared<box>(point3(0.0, 0.0, 0.0), 1.0, 0.1, 1.0, red_material)},
        {18, make_shared<box>(point3(0.0, 0.0, 0.0), 1.0, 0.1, 1.0, red_material)},
        // Paredes
        {19, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, brick_material, 1.5)},
        {20, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, brick_material, 1.5)},
        {21, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, brick_material, 1.5)},
        // Piso
        {22, make_shared<box>(point3(0.0, 0.0, 0.0), 6.0, 0.1, 10.0, wood_material, 3)},
    };

    for (const auto& [id, obj] : Atividade6) {
        world.add(obj, id);
    }

    applyAtividade6Transformations(world);
}

void SceneBuilder::applyAtividade6Transformations(SceneManager& world) {
    point3 viga_vmin = point3(0.5, 0.0, 0.0);
    point3 table_center = point3(1.25, 0.975, 0.75);

    Matrix4x4 movetable;
    movetable = movetable.translation(vec3(-1.25, 0, -5.75));
    Matrix4x4 movetable_origin;
    movetable_origin = movetable_origin.translation(-table_center);
    Matrix4x4 movetable_back;
    movetable_back = movetable_back.translation(table_center);

    Matrix4x4 movetree;
    movetree = movetree.translation(vec3(0, 1.0, -5));

    Matrix4x4 movewall;
    movewall = movewall.translation(vec3(3, 0, 0));

    Matrix4x4 movewall2;
    movewall2 = movewall2.translation(vec3(3, 0, -10));

    Matrix4x4 movefloor;
    movefloor = movefloor.translation(vec3(-3.0, 0.0, -10));

    Matrix4x4 shear;
    shear = shear.shearing(0.0, 0.75, 0.0);

    Matrix4x4 viga_scale;
    viga_scale = viga_scale.scaling(6.0, 1.0, 0.6);

    Matrix4x4 telhado_scale;
    telhado_scale = telhado_scale.scaling(4.5, 1.0, -9.7);

    Matrix4x4 parede_scale;
    parede_scale = parede_scale.scaling(0.2, 4.5, -10.0);

    Matrix4x4 parede_scale2;
    parede_scale2 = parede_scale2.scaling(0.2, 4.5, -6.0);

    Matrix4x4 move;
    move = move.translation(-viga_vmin);

    Matrix4x4 moveup;
    moveup = moveup.translation(vec3(-3.5, 4.5, 0));

    Matrix4x4 movefar;
    movefar = movefar.translation(vec3(0, 0, -10));

    Matrix4x4 pilar_move;
    pilar_move = pilar_move.translation(vec3(-3.5, 0.0, 0));

    Matrix4x4 moveback;
    moveback = moveback.translation(viga_vmin);

    Matrix4x4 rotate;
    rotate = rotate.rotation(37, 'Z');

    Matrix4x4 parede_rotate;
    parede_rotate = parede_rotate.rotation(90, 'y');

    Matrix4x4 portico_mirror;
    portico_mirror = portico_mirror.mirror(vec3(1,0,0), point3(0,0,0));

    Matrix4x4 mesa_transform = movetable_back * movetable * parede_rotate * movetable_origin;
    Matrix4x4 viga_transform = moveback * moveup * shear * viga_scale * move;
    Matrix4x4 telhado_transform = moveup * rotate * telhado_scale;
    Matrix4x4 telhado_transform2 = portico_mirror * moveup * rotate * telhado_scale;
    Matrix4x4 parede_transform = movewall * parede_scale;
    Matrix4x4 parede_transform2 = movewall2 * parede_rotate * parede_scale2;

    // Mesa
    world.transform_range(2, 4, mesa_transform);
    // Arvore de Natal
    world.transform_range(5, 8, movetree);
    // Portico
    world.transform_range(9, 10, pilar_move);
    world.transform_range(11, 12, viga_transform);
    world.transform_object(10, portico_mirror);
    world.transform_object(12, portico_mirror);

    world.transform_range(13, 14, pilar_move);
    world.transform_range(15, 16, viga_transform);
    world.transform_object(14, portico_mirror);
    world.transform_object(16, portico_mirror);

    world.transform_range(13, 16, movefar);
    // Telhado
    world.transform_object(17, telhado_transform);
    world.transform_object(18, telhado_transform2);
    // Parede
    world.transform_range(19, 20, parede_transform);
    world.transform_object(20, portico_mirror);
    world.transform_object(21, parede_transform2);
    // Piso
    world.transform_object(22, movefloor);
}

void SceneBuilder::buildSonicScene(SceneManager& world) {
    std::vector<std::shared_ptr<hittable>> Scene2 = {
    make_shared<plane>(point3(0, 0, 0), vec3(0, 1, 0), mat(grass_texture), 0.2),
    make_shared<box>(point3(-40, -0.5, -20), point3(-20, 15, -40), mat(&ground), 1.0, 0.9),
    make_shared<box>(point3(-40.8, 15, -19.2), point3(-19.2, 18, -40.8), mat(color(0.29, 0.71, 0))),
    make_shared<box>(point3(-15, -0.5, -20), point3(50, 5, -40), mat(&ground), 2, 0.2),
    make_shared<box>(point3(-16, 5, -19.2), point3(51, 7, -39.2), mat(color(0.29, 0.71, 0))),
    make_shared<sphere>(point3(-9.9, 13.5, -0.77), 1, mat(&checker)),
    make_shared<cylinder>(point3(8, 1.6, -4), 0.5, 1, mat(color(0.6, 0.6, 0.6), 0.8, 0.8, 100)),
    make_shared<cylinder>(point3(8, 2.1, -4), 0.5, 1.5, mat(yellow, 1.0, 1.0, 1000)),

    };

    //Add all objects to the manager with their manually assigned IDs
    for (const auto& obj : Scene2) {
        ObjectID id = world.add(obj);
        //std::cout << "Added object with ID " << id << ".\n";
    }

    auto torusOBJ = make_shared<torus>(point3(0, 1, 1), 0.5, 0.15, vec3(0.45, 0.0, 0.5), mat(yellow, 1, 1.0, 1000, 0.6));
    ObjectID torus = world.add(torusOBJ);
    duplicateObjectArray(world, torus, 4, 2, vec3(1, 0, 0), true);

    auto coneOBJ = make_shared<cone>(point3(-35, 0, -15), point3(-35, 3.5, -15), 0.5, mat(color(0.6, 0.6, 0.6), 0.8, 0.8, 100));
    ObjectID cone = world.add(coneOBJ);
    duplicateObjectArray(world, cone, 30, 2.5, vec3(1, 0, 0));

    //Mesh Objects

    try {
        ObjectID sonic = add_mesh_to_scene(AssetPath::Resolve("models/sonic.obj"), world, AssetPath::Resolve("models/sonic.mtl"));
        ObjectID totemID = add_mesh_to_scene(AssetPath::Resolve("models/cenario/totem.obj"), world, AssetPath::Resolve("models/cenario/totem.mtl"));
        ObjectID loopID = add_mesh_to_scene(AssetPath::Resolve("models/cenario/loop.obj"), world, AssetPath::Resolve("models/cenario/loop.mtl"));
        ObjectID palmID = add_mesh_to_scene(AssetPath::Resolve("models/cenario/palm.obj"), world, AssetPath::Resolve("models/cenario/palm.mtl"));

        if (world.contains(loopID)) {
            Matrix4x4 loopTranslate = loopTranslate.translation(vec3(0, 1, -6));
            world.transform_object(loopID, loopTranslate);
        }

        if (world.contains(palmID)) {
            Matrix4x4 palmTranslate = palmTranslate.translation(vec3(-20, 0, -12));
            world.transform_object(palmID, palmTranslate);
        }

        if (world.contains(totemID)) {
            Matrix4x4 totemTranslate = totemTranslate.translation(vec3(-10, 0, -1));
            world.transform_object(totemID, totemTranslate);
        }

        if (world.contains(sonic)) {
            point3 sonicCenter = world.get(sonic)->bounding_box().getCenter();
            Matrix4x4 sonicTranslate = sonicTranslate.translation(vec3(-2, 0.3, 1));
            Matrix4x4 sonicRotate = sonicRotate.rotateAroundPoint(sonicCenter, vec3(0, 1, 0), 90);
            Matrix4x4 sonicScale = sonicScale.scaleAroundPoint(sonicCenter, 1.2, 1.2, 1.2);
            Matrix4x4 sonicTransform = sonicTranslate * sonicScale * sonicRotate;
            world.transform_object(sonic, sonicTransform);
        }

        duplicateObjectArray(world, totemID, 1, 20, vec3(1, 0, 0));

        int numCopies = 4;      // Number of palm copies to spawn
        double distance = 10.0; // Distance in meters between each palm

        for (int i = 0; i < numCopies; i++) {
            // Ensure the original object exists in the world
            if (!world.contains(palmID)) {
                std::cerr << "Error: Palm object with ID " << palmID << " not found in world." << std::endl;
                break; // Exit loop if the object doesn't exist
            }

            // Clone the original palm object
            auto originalObject = world.get(palmID);
            auto newObject = originalObject->clone();

            // Alternate scale: if index is even, use 1.5; if odd, use 1.0.
            double scaleFactor = (i % 2 == 0) ? 1.5 : 1.0;

            // Create a translation matrix to place the new palm instance
            Matrix4x4 translate = Matrix4x4::translation(vec3(distance * (i + 1), 0.0, 0.0));

            // Compute scaling around the center of the mesh's bounding box
            Matrix4x4 scale = scale.scaleAroundPoint(
                originalObject->bounding_box().getCenter(),
                scaleFactor, scaleFactor, scaleFactor
            );

            // Combine translation and scaling into a single transformation
            Matrix4x4 transform = translate * scale;

            // Add the cloned object to the world and apply transformation
            ObjectID newPalmID = world.add(newObject); // Add new instance to world
            world.transform_object(newPalmID, transform);
        }

        auto palm1 = world.get(palmID)->clone();
        ObjectID palm1ID = world.add(palm1);
        Matrix4x4 palmTranslate = palmTranslate.translation(vec3(5, 0, 10));
        world.transform_object(palm1ID, palmTranslate);
        duplicateObjectArray(world, palm1ID, 1, 30, vec3(1, 0, 0));

    }
    catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << " - Skipping this model." << std::endl;
        return;
    }
}