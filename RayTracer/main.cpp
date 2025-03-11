
#include "raytracer.h"
#include "wireframe.h"

#include "camera.h"
#include "light.h"

//primitives
#include "sphere.h"
#include "plane.h"
#include "cylinder.h"
#include "cone.h"
#include "box.h"
#include "box_csg.h"
#include "mesh.h"
#include "torus.h"
#include "squarepyramid.h"

//modelling
#include "csg.h"
#include "octree.h"

//scene setup
#include "interface_imgui.h"
#include "sdl_setup.h"
#include "render_state.h"

//external libs
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

RenderState render_state;
bool renderWireframe = false;
bool renderWorldAxes = true;
std::optional<BoundingBox> highlighted_box = std::nullopt;

// Helper function that spawns an array of a mesh.
// Each copy is translated in the given direction by 'fixedDistance' meters per step.
auto static spawnMeshArray(SceneManager& world, const std::string& meshPath, const std::string& mtlPath, int numCopies, float fixedDistance, const vec3& direction) {
    for (int i = 0; i < numCopies; i++) {
        // Load a new copy of the mesh
        ObjectID meshID = add_mesh_to_scene(meshPath, world, mtlPath);
        if (world.contains(meshID)) {
            // Calculate translation based on direction vector
            vec3 offset = direction * (fixedDistance * (i + 1));
            Matrix4x4 translate = Matrix4x4::translation(offset);

            // Apply transformation to place the mesh
            world.transform_object(meshID, translate);
        }
    }
}

// Helper function that duplicates an existing object in the SceneManager
// and translates each copy along a specified direction.
auto static duplicateObjectArray(SceneManager& world, ObjectID originalID, int numCopies, float fixedDistance, const vec3& direction, bool applyRotation = false) {
    if (!world.contains(originalID)) {
        std::cerr << "Error: Original ObjectID " << originalID << " not found in SceneManager.\n";
        return;
    }

    // Retrieve the original object
    auto originalObject = world.get(originalID);
    if (!originalObject) {
        std::cerr << "Error: Failed to retrieve object with ID " << originalID << ".\n";
        return;
    }

    for (int i = 0; i < numCopies; i++) {
        // Clone the original object
        std::shared_ptr<hittable> newObject = originalObject->clone();
        if (!newObject) {
            std::cerr << "Error: Cloning failed for object ID " << originalID << ".\n";
            return;
        }

        // Add the new instance to the scene
        ObjectID newID = world.add(newObject);

        // Calculate translation offset
        vec3 offset = direction * (fixedDistance * (i + 1));
        Matrix4x4 translate = Matrix4x4::translation(offset);

        // Apply optional rotation
        Matrix4x4 rotate;
        if (applyRotation) {
            vec3 rotateDirection = vec3(0, 1, 0); // Example: Y-axis rotation
            float angle = 10.0f; // Example: Rotation angle increment
            rotate = rotate.rotateAroundPoint(world.get(newID)->bounding_box().getCenter(), rotateDirection, angle * (i + 5));
        }
        else {
            Matrix4x4 rotate;
        }

        Matrix4x4 transform = translate * rotate;

        // Apply transformation to the new instance
        world.transform_object(newID, transform);
    }
}


int main(int argc, char* argv[]) {

    // Image
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 720;
    int image_height = int(image_width / aspect_ratio);
    int window_width = 1080;
    int window_height = int(window_width / aspect_ratio);

    _putenv("KMP_WARNINGS=0");
    omp_set_max_active_levels(1);

    if (!InitializeSDL()) {
        return -1;
    }

    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = CreateWindow(window_width, window_height, "Raytracer CG1 - Abner Augusto");
    if (!window) {
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = CreateRenderer(window);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Texture* texture = CreateTexture(renderer, image_width, image_height);
    if (!texture) {
        Cleanup_SDL(window, renderer, nullptr);
        return -1;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    ImGui::StyleColorsDark();

    TTF_Font* font = TTF_OpenFont("fonts/Roboto-Medium.ttf", 16);
    if (font == nullptr) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Material list
    color black(0.0, 0.0, 0.0);
    color white(1.0, 1.0, 1.0);
    color red(1.0, 0.0, 0.0);
    color orange(1.0, 0.5, 0.0);
    color green(0.0, 1.0, 0.0);
    color blue(0.0, 0.0, 1.0);
    color cyan(0.0, 1.0, 0.9);
    color brown(0.69, 0.49, 0.38);
    color yellow(1, 1, 0);

    image_texture* wood_texture = new image_texture("textures/wood_floor.jpg");
    image_texture* grass_texture = new image_texture("textures/grass.jpg");
    image_texture* brick_texture = new image_texture("textures/brick.jpg");
    checker_texture checker(black, white, 15);
    checker_texture ground(color(0.43, 0.14, 0), color(0.86, 0.43, 0), 20);
    mat xadrez(&checker, 0.8, 1.0, 100.0, 0.25);
    mat sphere_mat(red, 0.8, 1.0, 150.0);
    mat sphere_mat2(green);
    mat plane_material(brown, 0.3, 0.3, 2.0);
    mat reflective_material(black, 0.8, 1.0, 150.0, 0.1);
    mat voxel_material(color(1, 1, 1));

    // Class that holds all objects related to scene
    SceneManager world;

    // Create scenes
    std::vector<std::shared_ptr<hittable>> Scene1 = {
        std::make_shared<plane>(point3(0, -0.3, 0), vec3(0, 1, 0), mat(grass_texture), 0.4),
        make_shared<box>(point3(0, -0.5, -200), point3(240, -0.2, -50), mat(white))
    };

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
        //ObjectID mesh = add_mesh_to_scene("models/garanhuns.obj", world, "models/garanhuns.mtl");

        ObjectID sonic = add_mesh_to_scene("models/sonic.obj", world, "models/sonic.mtl");
        ObjectID totemID = add_mesh_to_scene("models/cenario/totem.obj", world, "models/cenario/totem.mtl");
        ObjectID loopID = add_mesh_to_scene("models/cenario/loop.obj", world, "models/cenario/loop.mtl");
        ObjectID palmID = add_mesh_to_scene("models/cenario/palm.obj", world, "models/cenario/palm.mtl");

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
        return {};
    }

    //Lights
    
    //Sonic
    world.add_directional_light(vec3(-0.6, -0.38, -0.7), 0.85, color(1, 1, 1));
    world.add_point_light(point3(6.2, 0.15, 0.5), 1.3, color(1, 0.87, 0.12));

    ////Garanhuns
    //world.add_directional_light(vec3(-0.6, -0.5, -0.5), 0.9, color(0.28, 0.43, 0.9));

    //world.add_point_light(point3(115.4, 4, -35), 1.0, color(1, 0.9, 0.9));
    //world.add_point_light(point3(117.4, 1.2, -50), 1.8, color(1, 0.69, 0.44));

    //// Left Spots
    //world.add_spot_light(point3(99.55, 0.0, -52.9), vec3(0,0.6,-0.8), 2.1, color(0.56, 0, 1), 20, 47);
    //world.add_spot_light(point3(88.55, 0.0, -52.9), vec3(0, 0.6, -0.8), 2.1, color(0.56, 1, 1), 20, 47);

    //// Right Spots
    //world.add_spot_light(point3(133.55, 0.0, -52.9), vec3(0, 0.6, -0.8), 2.1, color(0.56, 1, 1), 20, 47);
    //world.add_spot_light(point3(146.55, 0.0, -52.9), vec3(0, 0.6, -0.8), 2.1, color(0.56, 0, 1), 20, 47);

    // Camera
    double camera_fov = 60;
    double viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto samples_per_pixel = 5; 
    float speed = 1.5f;
    //point3 origin(98,4.8, -17.0);
    //point3 look_at(100 , 5, -21);
    point3 origin(-1.4, 3.4, 16.2);
    point3 look_at(-1.2 , 7.7, -3);


    Camera camera(
        origin,
        look_at,
        image_width,
        aspect_ratio,
        camera_fov
    );

    // World to Camera
    if (camera.CameraSpaceStatus()) {
        std::cout << "Calling World to Camera Transform" << ".\n";
        world.transform(camera.world_to_camera_matrix);
    }

    //BG Color (custom for garanhuns)
    //camera.set_BGhorizon(color(0.95, 0.45, 0.25));
    //camera.set_BGtop(color(0.07, 0.27, 0.64));


    //Build a BVH Tree
    world.buildBVH();

    // FPS Counter
    float deltaTime = 0.0f;
    Uint64 currentTime = SDL_GetPerformanceCounter();
    Uint64 lastTime = 0;
    SDL_Rect destination_rect{};

    // Render loop
    bool running = true;
    SDL_Event event;
    while (running) {
        lastTime = currentTime;
        currentTime = SDL_GetPerformanceCounter();
        deltaTime = (float)((currentTime - lastTime) * 1000 / (double)SDL_GetPerformanceFrequency());
        float fps = 1000.0f / deltaTime;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            handle_event(event, running, window, aspect_ratio, camera, render_state, world, highlighted_box, speed);
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        draw_menu(render_state,
            camera,
            world);

        DrawFpsCounter(fps);

        ShowHittableManagerUI(world, camera);

        // Render ImGui
        ImGui::Render();

        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

        //Get previous render state
        RenderMode previous_mode = render_state.get_previous_mode();

        update_camera(camera, 0.2f);

        // Render raytraced scene
        if (render_state.is_mode(DefaultRender)) {
            if (previous_mode != DefaultRender) {
                render_state.set_mode(DefaultRender);
                camera.set_image_width(image_width);

                SDL_DestroyTexture(texture);
                texture = SDL_CreateTexture(
                    renderer,
                    SDL_PIXELFORMAT_ARGB8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    camera.get_image_width(),
                    camera.get_image_height()
                );
            }

            camera.render(world, samples_per_pixel, false);
        }
        else if (render_state.is_mode(HighResolution)) {
            Uint64 start_time = SDL_GetPerformanceCounter();
            camera.set_image_width(1920);
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                camera.get_image_width(),
                camera.get_image_height()
            );

            camera.render(world, samples_per_pixel, false);
            Uint64 end_time = SDL_GetPerformanceCounter();
            double render_time = (end_time - start_time) / (double)SDL_GetPerformanceFrequency();

            // Print resolution along with render time
            std::cout << "High-Resolution Render: "
                << camera.get_image_width() << "x" << camera.get_image_height()
                << " | Render Time: " << render_time << " seconds" << std::endl;

            render_state.set_mode(Disabled);
        }
        else if (render_state.is_mode(LowResolution)) {
            Uint64 start_time = SDL_GetPerformanceCounter();
            camera.set_image_width(640);
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                camera.get_image_width(),
                camera.get_image_height()
            );

            camera.render(world, samples_per_pixel * 2, true);
            Uint64 end_time = SDL_GetPerformanceCounter();
            double render_time = (end_time - start_time) / (double)SDL_GetPerformanceFrequency();

            // Print resolution along with render time
            std::cout << "Low-Resolution Render: "
                << camera.get_image_width() << "x" << camera.get_image_height()
                << " | Render Time: " << render_time << " seconds" << std::endl;

            render_state.set_mode(Disabled);
        }

        Uint32* pixels = camera.get_pixels();
        SDL_GetWindowSize(window, &window_width, &window_height);

        // Calculate the destination rectangle to properly scale and center the texture
        int texture_width = camera.get_image_width();
        int texture_height = camera.get_image_height();
        double texture_aspect_ratio = static_cast<double>(texture_width) / texture_height;
        double window_aspect_ratio = static_cast<double>(window_width) / window_height;

        if (texture_aspect_ratio > window_aspect_ratio) {
            // Texture is wider than the window
            destination_rect.w = window_width;
            destination_rect.h = static_cast<int>(window_width / texture_aspect_ratio);
            destination_rect.x = 0;
            destination_rect.y = (window_height - destination_rect.h) / 2; // Center vertically
        }
        else {
            // Texture is taller than the window
            destination_rect.w = static_cast<int>(window_height * texture_aspect_ratio);
            destination_rect.h = window_height;
            destination_rect.x = (window_width - destination_rect.w) / 2; // Center horizontally
            destination_rect.y = 0;
        }
        SDL_UpdateTexture(texture, nullptr, pixels, camera.get_image_width() * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, &destination_rect);

        DrawCrosshair(renderer, window_width, window_height);

        if (renderWorldAxes) {
            render_world_axes(renderer, camera, destination_rect);
        }

        if (renderWireframe) {
            DrawOctreeWireframe(renderer, world, camera, destination_rect, highlighted_box);
        }

        // Render ImGui
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        // Present the final frame
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    Cleanup_SDL(window, renderer, texture);

    TTF_CloseFont(font);

    return 0;
}