
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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

RenderState render_state;
bool renderWireframe = false;
bool renderWorldAxes = true;
std::optional<BoundingBox> highlighted_box = std::nullopt;

int main(int argc, char* argv[]) {

    // Image
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 720;
    int image_height = int(image_width / aspect_ratio);
    int window_width = 1080;
    int window_height = int(window_width / aspect_ratio);

    omp_set_max_active_levels(1);

    if (!InitializeSDL()) {
        return -1;
    }

    SDL_Window* window = CreateWindow(window_width, window_height, "Modelagem CSG - Abner Augusto");
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

    // Material list
    color black(0.0, 0.0, 0.0);
    color white(1.0, 1.0, 1.0);
    color red(1.0, 0.0, 0.0);
    color orange(1.0, 0.5, 0.0);
    color green(0.0, 1.0, 0.0);
    color blue(0.0, 0.0, 1.0);
    color cyan(0.0, 1.0, 0.9);
    color brown(0.69, 0.49, 0.38);

    image_texture* wood_texture = new image_texture("textures/wood_floor.jpg");
    image_texture* grass_texture = new image_texture("textures/grass.jpg");
    image_texture* brick_texture = new image_texture("textures/brick.jpg");
    checker_texture checker(black, white, 15);
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
        std::make_shared<plane>(point3(0, -0.5, 0), vec3(0, 1, 0), mat(grass_texture)),
        make_shared<sphere>(point3(0, 0, -1), 0.45, mat(xadrez)),
        make_shared<cylinder>(point3(-1.0, -0.25, -1), point3(-1.0, 0.35, -1), 0.3, mat(blue)),
        make_shared<cone>(point3(1, -0.15, -1), point3(1, 0.5, -1.5), 0.3, mat(red)),
        make_shared<torus>(point3(-2, 0, -1), 0.3, 0.1, vec3(0, 0.5, 0.5), mat(cyan)),
        make_shared<SquarePyramid>(point3(1.8, -0.3, -1), 0.8, 0.5, mat(green))
    };

    std::vector<std::shared_ptr<hittable>> Scene2 = {
    std::make_shared<plane>(point3(0, -0.5, 0), vec3(0, 1, 0), mat(grass_texture)),
    make_shared<torus>(point3(-2, 0, -1), 0.3, 0.1, vec3(0, 0.5, 0.5), mat(cyan)),
    };

    //Add all objects to the manager with their manually assigned IDs
    for (const auto& obj : Scene1) {
        ObjectID id = world.add(obj);
        //std::cout << "Added object with ID " << id << ".\n";
    }

    //Mesh Object

    MeshOBJ mesh;
    try {
        // Load an OBJ mesh and add it to the manager
        mesh = add_mesh_to_scene("models/sonic.obj", world, "models/sonic.mtl");

        // Apply transformation to all triangles in the mesh
        if (!mesh.triangle_ids.empty()) {

            Matrix4x4 translate = Matrix4x4::translation(vec3(0.0, 0, -2.0));
            Matrix4x4 scale = Matrix4x4::scaling(0.5, 0.5, 0.5);
            Matrix4x4 translate_origin = Matrix4x4::translation(-mesh.first_vertex.value());
            Matrix4x4 translate_back = Matrix4x4::translation(mesh.first_vertex.value());
            //Matrix4x4 rotate_mesh = Matrix4x4::rotation(15, 'Y');
            //Matrix4x4 shear = Matrix4x4::shearing(0.1, 0.3, 0);
            Matrix4x4 mesh_transform = translate * translate_back *  scale * translate_origin;

            for (ObjectID id : mesh.triangle_ids) {
                world.transform_object(id, translate);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }


    // Build Atividade 6 Scene
    //SceneBuilder::buildAtividade6Scene(
    //    world,
    //    mat(grass_texture),
    //    mat(orange),
    //    mat(white),
    //    mat(brown),
    //    mat(green),
    //    mat(red),
    //    mat(brick_texture),
    //    mat(wood_texture)
    //);

    //Light
    world.add_directional_light(vec3(-1, -1, 0), 0.5, color(1, 0.95, 0.8));
    world.add_point_light(vec3(0, 1, 0.5), 1.0, color(1, 1, 1));
    //world.add_point_light(vec3(0, 3, -2), 1.0, color(1, 1, 1));


    // Cameran
    double camera_fov = 60;
    double viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto samples_per_pixel = 5; 
    //point3 origin(-2.2, 1.2, 2.7);
    //point3 look_at(0.5 , 0.0, -1.5);
    point3 origin(0, 0, 2);
    point3 look_at(0 , 0, -3);


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
            handle_event(event, running, window, aspect_ratio, camera, render_state, world, highlighted_box);
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        draw_menu(render_state,
            camera,
            world);

        DrawFpsCounter(fps);

        ShowHittableManagerUI(world);

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
            camera.set_image_width(1280);
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                camera.get_image_width(),
                camera.get_image_height()
            );

            camera.render(world,samples_per_pixel, true);
            Uint64 end_time = SDL_GetPerformanceCounter();
            double render_time = (end_time - start_time) / (double)SDL_GetPerformanceFrequency();
            std::cout << "render time: " << render_time << " seconds" << std::endl;
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
            std::cout << "render time: " << render_time << " seconds" << std::endl;
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

    return 0;
}