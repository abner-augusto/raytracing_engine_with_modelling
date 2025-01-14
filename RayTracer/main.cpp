
#include "raytracer.h"

#include "camera.h"
#include "light.h"

//primitives
#include "sphere.h"
#include "plane.h"
#include "cylinder.h"
#include "cone.h"
#include "box.h"
#include "mesh.h"
#include "interface_imgui.h"

//scene setup
#include "sdl_setup.h"
#include "render_state.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL2/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

RenderState render_state;
bool isCameraSpace = true;

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
    color orange(1.0, 0.5, 0.0);
    color green(0.0, 1.0, 0.0);
    color blue(0.0, 0.0, 1.0);
    color cyan(0.0, 1.0, 0.9);
    color brown(0.69, 0.49, 0.38);

    image_texture* wood_texture = new image_texture("textures/wood_floor.jpg");
    image_texture* grass_texture = new image_texture("textures/grass.jpg");
    image_texture* brick_texture = new image_texture("textures/brick.jpg");
    checker_texture checker(black, white, 3.0);
    mat xadrez(&checker, 0.8, 1.0, 100.0, 0.25);
    mat sphere_mat(red, 0.8, 1.0, 150.0);
    mat sphere_mat2(green);
    mat plane_material(brown, 0.3, 0.3, 2.0);
    mat reflective_material(black, 0.8, 1.0, 150.0, 0.5);
    mat voxel_material(color(1, 1, 1));

    // World Scene
    HittableManager world;

    // Create scenes
    std::vector<std::pair<ObjectID, std::shared_ptr<hittable>>> Scene1 = {
        {1, make_shared<plane>(point3(0, -0.50, 0), vec3(0, 1, 0), mat(wood_texture))},
        {2, make_shared<sphere>(point3(0, -0.15, -2), 0.3, xadrez)},
        {3, make_shared<cylinder>(point3(-1.0, -0.15, -2), point3(-0.9, 0.25, -1.6), 0.3, green)},
        {4, make_shared<cone>(point3(1, -0.15, -2), point3(1, 0.5, -2.5), 0.3, red, true)},
        {5, make_shared<box>(point3(2.3, 0.3, -2.0), point3(1.5, -0.50, -1.5), mat(white))}
    };

    //Mesh Object
    /*try {
        // Load an OBJ mesh and add it to the manager
        ObjectID mesh_id = add_mesh_to_manager("models/prism.obj", world, blue);
        Matrix4x4 translate;
        translate = translate.translation(-3.5, 0.0, 0.0);
        world.transform_object(mesh_id, translate);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }*/

    std::vector<std::pair<ObjectID, std::shared_ptr<hittable>>> Atividade6 = {
    {1, make_shared<plane>(point3(0, 0, 0), vec3(0, 1, 0), mat(grass_texture), 0.5)},
    //Mesa
    {2, make_shared<box>(point3(0.0, 0.95, 0.0), 2.5, 0.05, 1.5, mat(orange))},
    {3, make_shared<box>(point3(0.0, 0.0, 0.0), 0.05, 0.95, 1.5, mat(white))},
    {4, make_shared<box>(point3(2.45, 0.0, 0.0), 0.05, 0.95, 1.5, mat(white))},
    //Arvore de Natal
    {5, make_shared<cylinder>(point3(0, 0.0, 0.0), point3(0, 0.09, 0), 0.3, mat(brown))},
    {6, make_shared<cylinder>(point3(0, 0.09, 0.0), point3(0, 0.49, 0), 0.06, mat(brown))},
    {7, make_shared<cone>(point3(0, 0.49, 0.0), point3(0, 1.99, 0), 0.60, mat(green))},
    {8, make_shared<sphere>(point3(0, 2.0, 0.0), 0.045, mat(red))},
    //Portico 1
    {9, make_shared<box>(point3(0, 0.0, 0.0), 0.5, 5.0, 0.3, mat(white))},
    {10, make_shared<box>(point3(0.0, 0.0, 0.0), 0.5, 5.0, 0.3, mat(white))},
    {11, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, mat(white))},
    {12, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, mat(white))},
    //Portico 2
    {13, make_shared<box>(point3(0, 0.0, 0.0), 0.5, 5.0, 0.3, mat(white))},
    {14, make_shared<box>(point3(0.0, 0.0, 0.0), 0.5, 5.0, 0.3, mat(white))},
    {15, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, mat(white))},
    {16, make_shared<box>(point3(0.5, 0.0, 0.0), 0.5, 0.5, 0.5, mat(white))},
    //Telhado
    {17, make_shared<box>(point3(0.0, 0.0, 0.0), 1.0, 0.1, 1.0, mat(red))},
    {18, make_shared<box>(point3(0.0, 0.0, 0.0), 1.0, 0.1, 1.0, mat(red))},
    //Paredes
    {19, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, mat(brick_texture), 1.5)},
    {20, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, mat(brick_texture), 1.5)},
    {21, make_shared<box>(point3(0.0, 0.0, 0), 1.0, 1.0, 1.0, mat(brick_texture), 1.5)},
    //Piso
    {22, make_shared<box>(point3(0.0, 0.0, 0.0), 6.0, 0.1, 10.0, mat(wood_texture), 3)},
    };

    // Add all objects to the manager with their manually assigned IDs
    for (const auto& [id, obj] : Atividade6) {
        world.add(obj, id);
        //std::cout << "Added object with ID " << id << ".\n";
    }

    // Group objects
    std::vector<ObjectID> portico_ids = { 9, 10, 11, 12 };
    ObjectID portico_group1 = world.create_group(portico_ids);

    std::vector<ObjectID> portico_ids2 = { 13, 14, 15, 16 };
    ObjectID portico_group2 = world.create_group(portico_ids2);

    // Matrix List
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

    Matrix4x4 shear2;
    shear2 = shear2.shearing(0.0, 0.75, 0.0);

    Matrix4x4 viga_scale;
    viga_scale = viga_scale.scaling(6.0, 1.0, 0.6);

    Matrix4x4 telhado_scale;
    telhado_scale = telhado_scale.scaling(4.5, 1.0, -10.0);

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
    portico_mirror = portico_mirror.mirror('y');

    Matrix4x4 mesa_transform = movetable_back * movetable * parede_rotate * movetable_origin;

    Matrix4x4 viga_transform = moveback * moveup * shear * viga_scale * move;

    Matrix4x4 telhado_transform = moveup * rotate * telhado_scale;
    Matrix4x4 telhado_transform2 = portico_mirror * moveup * rotate * telhado_scale;

    Matrix4x4 parede_transform = movewall * parede_scale;
    Matrix4x4 parede_transform2 = movewall2 * parede_rotate * parede_scale2;

    //Mesa
    world.transform_range(2, 4, mesa_transform);
    //Arvore de Natal
    world.transform_range(5, 8, movetree);
    //Portico
    world.transform_range(9, 10, pilar_move);
    world.transform_range(11, 12, viga_transform);
    world.transform_object(10, portico_mirror);
    world.transform_object(12, portico_mirror);

    world.transform_range(13, 14, pilar_move);
    world.transform_range(15, 16, viga_transform);
    world.transform_object(14, portico_mirror);
    world.transform_object(16, portico_mirror);

    world.transform_object(portico_group2, movefar);
    //Telhado
    world.transform_object(17, telhado_transform);
    world.transform_object(18, telhado_transform2);
    //Parede
    world.transform_range(19, 20, parede_transform);
    world.transform_object(20, portico_mirror);
    world.transform_object(21, parede_transform2);
    //Piso
    world.transform_object(22, movefloor);

    //Light
    std::vector<Light> lights = {
        Light(vec3(0.0, 3, -4), 1.2, color(1.0, 1.0, 1.0)),
        Light(vec3(-5, 4.5, 0), 1.5, color(1.0, 0.82, 0.20)),
        //Light(vec3(2.5, 6, -1), 2.0, color(0.3, 0.3, 1.0))
    };

    // Camera
    double camera_fov = 60;
    double viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto samples_per_pixel = 4; 
    point3 origin(-4.1, 4.3, 6.9);
    point3 look_at(-1.8 , 3.7, 3.0);

    Camera camera(
        origin,
        look_at,
        image_width,
        aspect_ratio,
        camera_fov
    );

    // World to Camera
    if (isCameraSpace) {
        std::cout << "Calling World to Camera Transform" << ".\n";
        camera.transform_scene_and_lights(world, lights);
    }

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
            handle_event(event, running, window, aspect_ratio, camera, render_state);
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        draw_menu(render_state,
            camera,
            world, 
            lights);

        DrawFpsCounter(fps);

        // Render ImGui
        ImGui::Render();

        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

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

            camera.render(world, lights, samples_per_pixel, false, isCameraSpace);
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

            camera.render(world, lights, samples_per_pixel, true, isCameraSpace);
            Uint64 end_time = SDL_GetPerformanceCounter();
            double render_time = (end_time - start_time) / (double)SDL_GetPerformanceFrequency();
            std::cout << "render time: " << render_time << " seconds" << std::endl;
            render_state.set_mode(Disabled);
        }
        else if (render_state.is_mode(LowResolution)) {
            Uint64 start_time = SDL_GetPerformanceCounter();
            camera.set_image_width(480);
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                camera.get_image_width(),
                camera.get_image_height()
            );

            camera.render(world, lights, samples_per_pixel, false, isCameraSpace);
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