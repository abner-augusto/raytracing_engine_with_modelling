
#include "raytracer.h"

#include "camera.h"
#include "light.h"
#include "sphere.h"
#include "plane.h"
#include "cylinder.h"
#include "cone.h"
#include "torus.h"
#include "box.h"
#include "boundingbox.h"
#include "node.h"
#include "wireframe.h"
#include "interface_imgui.h"
#include "sdl_setup.h"
#include "octreemanager.h"
#include "render_state.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL2/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

RenderState render_state;

int main(int argc, char* argv[]) {
    // Image
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 480;
    int image_height = int(image_width / aspect_ratio);
    int window_width = 1080;
    int window_height = int(window_width / aspect_ratio);

    if (!InitializeSDL()) {
        return -1;
    }

    SDL_Window* window = CreateWindow(window_width, window_height, "Raytracing CG1");
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    ImGui::StyleColorsDark();

    // Material list
    color black(0.0, 0.0, 0.0);
    color white(1.0, 1.0, 1.0);
    color red(1.0, 0.0, 0.0);
    color green(0.0, 1.0, 0.0);
    color blue(0.0, 0.0, 1.0);
    color brown(0.69, 0.49, 0.38);

    image_texture* wood_texture = new image_texture("textures/wood_floor.jpg");
    checker_texture checker(black, white, 15.0);
    mat wood_material(wood_texture);
    mat xadrez(&checker, 0.8, 1.0, 100.0, 0.25);
    mat sphere_mat(red, 0.8, 1.0, 150.0);
    mat sphere_mat2(green);
    mat plane_material(brown, 0.3, 0.3, 2.0);
    mat reflective_material(black, 0.8, 1.0, 150.0, 0.5);
    mat voxel_material(color(1, 1, 1));

    //Octree
    OctreeManager octreeManager;

    // World Scene
    hittable_list world;
    auto moving_sphere = make_shared<sphere>(point3(0, 0.5, -3), 0.5, sphere_mat);
    world.add(moving_sphere);
    world.add(make_shared<sphere>(point3(-0.9, -0.15, -2), 0.3, xadrez));
    world.add(make_shared<plane>(point3(0, -0.5, 0), vec3(0, 1, 0), wood_material));
    world.add(make_shared<cylinder>(point3(0.7, -0.15, -2), point3(0.7, .3, -1.6), 0.3, sphere_mat2));
    world.add(make_shared<cone>(point3(0, -0.5, -2),point3(0, 0.5, -2), 0.5, sphere_mat, true));
    world.add(make_shared<torus>(point3(-2, 0.1, -2), 0.4, 0.18, vec3(0.5, 0.8, 0.8), reflective_material));

    //Light
    std::vector<Light> lights = {
        Light(vec3(-2, 2, -2), 1.0, color(1.0, 1.0, 1.0)),
        Light(vec3(2, 2, -2), 2.0, color(0.3, 0.3, 1.0))
    };

    // Camera
    double camera_fov = 60;
    double viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto samples_per_pixel = 10; 
    point3 origin(0, 0, 0);

    Camera camera(
        origin,
        image_width,
        aspect_ratio,
        camera_fov
    );

    // Sphere movement parameters
    double time = 0.0;
    double speed = 10.0;
    double amplitude = 0.4;

    // FPS Counter
    float deltaTime = 0.0f;
    Uint64 currentTime = SDL_GetPerformanceCounter();
    Uint64 lastTime = 0;
    SDL_Rect destination_rect{};

    bool show_wireframe = true;

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
            handle_event(event, running, window, aspect_ratio);
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        draw_menu(render_state,
            show_wireframe,
            speed,
            camera,
            origin,
            world);

        DrawFpsCounter(fps);
        RenderOctreeList(octreeManager);
        RenderOctreeInspector(octreeManager, world);

        // Render ImGui
        ImGui::Render();

        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

        // Animate sphere position
        time += 0.01;
        point3 sphereCenter((amplitude * 3) * sin(speed * time), 0.45 - amplitude * abs(sin(speed * time)), -3);
        moving_sphere->set_center(sphereCenter);
        //Get previous render state
        RenderMode previous_mode = render_state.get_previous_mode();

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

            camera.render(world, lights, samples_per_pixel, false);
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

            camera.render(world, lights, samples_per_pixel, true);
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


        // Render wireframe if enabled
        if (show_wireframe) {
            DrawOctreeManagerWireframe(
                renderer,
                octreeManager,
                camera,
                destination_rect
            );
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