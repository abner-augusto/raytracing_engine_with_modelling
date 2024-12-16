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

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL2/SDL.h>


int main(int argc, char* argv[]) {
    // Image
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 480;
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;
    int window_width = 1080;
    int window_height = int(window_width / aspect_ratio);
    // Allocate pixel buffer
    Uint32* pixels = new Uint32[image_width * image_height];

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
    color red(1.0, 0.0, 0.0);
    color green(0.0, 1.0, 0.0);
    color blue(0.0, 0.0, 1.0);
    color brown(0.69, 0.49, 0.38);

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
    world.add(make_shared<sphere>(point3(-0.9, -0.15, -2), 0.3, reflective_material));
    world.add(make_shared<plane>(point3(0, -0.5, 0), vec3(0, 1, 0), plane_material));
    world.add(make_shared<cylinder>(point3(0.7, -0.15, -2), point3(0.7, .3, -1.6), 0.3, sphere_mat2));
    world.add(make_shared<cone>(point3(0, -0.5, -2),point3(0, 0.5, -2), 0.5, sphere_mat, true));
    world.add(make_shared<torus>(point3(-2, 0.1, -2), 0.4, 0.18, vec3(0.5, 0.8, 0.8), reflective_material));



    //Light
    std::vector<Light> lights = {
        Light(vec3(-1, 2, -2), 1.0, color(1.0, 1.0, 1.0)),
        Light(vec3(2, 2, -2), 0.7, color(0.5, 0.5, 1.0))
    };

    // Camera
    double camera_fov = 60;
    double viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = degrees_to_radians(camera_fov);

    auto origin = point3(0, 0, 0);
    auto horizontal = vec3(viewport_width, 0, 0);
    auto vertical = vec3(0, viewport_height, 0);
    auto lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);

    Camera camera(
        origin,
        horizontal,
        vertical,
        lower_left_corner,
        focal_length,
        aspect_ratio
    );

    // Sphere movement parameters
    double time = 0.0;
    double speed = 10.0;
    double amplitude = 0.4;

    // FPS Counter
    float deltaTime = 0.0f;
    Uint64 currentTime = SDL_GetPerformanceCounter();
    Uint64 lastTime = 0;

    bool render_raytracing = false;
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

            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                    running = false;
                }
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int new_width = event.window.data1;
                    int new_height = event.window.data2;

                    double target_aspect_ratio = static_cast<double>(image_width) / image_height;
                    int viewport_width, viewport_height;

                    double window_aspect_ratio = static_cast<double>(new_width) / new_height;
                    if (window_aspect_ratio > target_aspect_ratio) {
                        viewport_height = new_height;
                        viewport_width = static_cast<int>(new_height * target_aspect_ratio);
                    }
                    else {
                        viewport_width = new_width;
                        viewport_height = static_cast<int>(new_width / target_aspect_ratio);
                    }

                    SDL_SetWindowSize(window, viewport_width, viewport_height);
                }
            }
        }

        // Zera o buffer de pixels antes de renderizar o próximo frame
        std::fill(pixels, pixels + (image_width * image_height), 0);

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        draw_menu(render_raytracing,
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

        // Render
        if (render_raytracing) {
            camera.render(pixels, image_width, image_height, world, lights);
        }

        // Update texture with pixel data
        SDL_UpdateTexture(texture, nullptr, pixels, image_width * sizeof(Uint32));
        // Render to window
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);

        if (show_wireframe) {
            DrawOctreeManagerWireframe(
                renderer, 
                window, 
                octreeManager,
                camera, 
                image_width, 
                image_height
            );

        }
        // Render ImGui
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    delete[] pixels;
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    Cleanup_SDL(window, renderer, texture);

    return 0;
}
