#include "raytracer.h"

#include "light.h"
#include "sphere.h"
#include "plane.h"
#include "cylinder.h"
#include "box.h"
#include "boundingbox.h"
#include "node.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL2/SDL.h>

static point3 random_position() {
    return point3(
        random_double(-2.0, 2.0),  // x
        -0.15,                     // y 
        random_double(-3.5, -1.0)  // z
    );
}

static color phong_shading(const hit_record& rec, const vec3& view_dir, const std::vector<Light>& lights, const hittable& world) {
    // Parâmetros de luz ambiente
    double ambient_light_intensity = 0.5;
    double k_ambient = 0.8;

    // Componente ambiente
    color ambient = k_ambient * ambient_light_intensity * rec.material->diffuse_color;

    // Inicializa cores difusa e especular
    color diffuse(0, 0, 0);
    color specular(0, 0, 0);

    // Loop através de cada luz e acumula contribuições difusas e especulares
    for (const auto& light : lights) {
        // Calcula a direção da luz e a distância até a luz
        vec3 light_dir = unit_vector(light.position - rec.p);
        double light_distance = (light.position - rec.p).length();

        ray shadow_ray(rec.p + rec.normal * 1e-3, light_dir);
        hit_record shadow_rec;

        // Se houver uma interseção com qualquer objeto antes de alcançar a luz, pula a contribuição desta luz
        if (world.hit(shadow_ray, interval(0.001, light_distance), shadow_rec)) {
            continue;
        }

        // Componente difusa
        double diff = std::max(dot(rec.normal, light_dir), 0.0);
        diffuse += rec.material->k_diffuse * diff * rec.material->diffuse_color * light.light_color * light.intensity;

        // Componente especular (não multiplica pela cor difusa)
        vec3 reflect_dir = reflect(-light_dir, rec.normal);
        double spec = std::pow(std::max(dot(view_dir, reflect_dir), 0.0), rec.material->shininess);
        specular += rec.material->k_specular * spec * light.light_color * light.intensity;
    }

    // Combina os componentes
    color final_color = ambient + diffuse + specular;
    // Limita os valores da cor entre 0 e 1
    final_color = clamp(final_color, 0.0, 1.0);

    return final_color;
}

static color cast_ray(const ray& r, const hittable& world, const std::vector<Light>& lights, int depth = 5) {
    if (depth <= 0) {
        return color(0, 0, 0);
    }

    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        vec3 view_dir = unit_vector(-r.direction());

        // Obtém a cor Phong para o objeto atingido
        color phong_color = phong_shading(rec, view_dir, lights, world);

        // Calcula reflexão se o material suportar
        if (rec.material->reflection > 0.0) {
            vec3 reflected_dir = reflect(unit_vector(r.direction()), rec.normal);
            ray reflected_ray(rec.p + rec.normal * 1e-3, reflected_dir);

            color reflected_color = cast_ray(reflected_ray, world, lights, depth - 1);

            // Combina a cor Phong com a reflexão
            return (1.0 - rec.material->reflection) * phong_color +
                rec.material->reflection * reflected_color;
        }

        return phong_color;
    }

    // Background gradient
    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.3 * (unit_direction.y() + 1.5);
    return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
}


int main(int argc, char* argv[]) {
    // Image
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 480;

    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;
    // Set SDL Hint to allow mouse focus click-through
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    // SDL Initialization
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }


    // SDL Initialization
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create a resizable window
    SDL_Window* window = SDL_CreateWindow("Raytracing CG1 Com Modelagem - Abner Augusto", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, image_width, image_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_RenderSetLogicalSize(renderer, image_width, image_height);

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, image_width, image_height);
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
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

    // Allocate pixel buffer
    Uint32* pixels = new Uint32[image_width * image_height];

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
    // Create a root bounding box
    BoundingBox root_bb(point3(-0.9, -0.45, -2.5), 1.5);
    // Create a sphere in the center of the bounding box
    sphere sp(root_bb.Center(), 0.6, mat());
    // Build the octree node from the bounding box and the sphere
    Node root = Node::FromObject(root_bb, sp, 3);
    // Get filled bounding boxes from the constructed node
    std::vector<BoundingBox> filled_bbs = root.GetFilledBoundingBoxes(root_bb);

    std::cout << "Filled Bounding Boxes:\n";
    for (auto& bb : filled_bbs) {
        std::cout << "Corner: ("
            << bb.vmin.x() << ", " << bb.vmin.y() << ", " << bb.vmin.z()
            << "), Width: " << bb.width << "\n";
    }

    // World Scene
    hittable_list world;
    //auto moving_sphere = make_shared<sphere>(point3(0, 0.5, -1), 0.5, sphere_mat);
    //world.add(moving_sphere);
    //world.add(make_shared<sphere>(point3(-0.9, -0.15, -1), 0.3, reflective_material));
    world.add(make_shared<plane>(point3(0, -0.5, 0), vec3(0, 1, 0), plane_material));
    //world.add(make_shared<cylinder>(point3(0.7, -0.15, -0.9), point3(0.7, .3, -0.6), 0.3, sphere_mat2, true));
    for (auto& bb : filled_bbs) {
        world.add(std::make_shared<box>(bb.vmin, bb.vmax(), voxel_material));
    }

    //Light
    std::vector<Light> lights = {
        Light(vec3(-2, 1.5, -0.2), 1.0, color(1.0, 0.5, 0.5)),
        //Light(vec3(2, 2, -2), 0.7, color(0.5, 0.5, 1.0))
    };

    // Camera
    auto viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = 1.0;

    auto origin = point3(0, 0, 0);
    auto horizontal = vec3(viewport_width, 0, 0);
    auto vertical = vec3(0, viewport_height, 0);
    auto lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);

    float camera_origin[3] = { 0.0f, 0.0f, 0.0f };
    float camera_fov = 90.0f;

    // Sphere movement parameters
    double time = 0.0;
    double speed = 20.0;
    double amplitude = 0.25;

    // FPS Counter
    float deltaTime = 0.0f;
    Uint64 currentTime = SDL_GetPerformanceCounter();
    Uint64 lastTime = 0;

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
        //ImGui::ShowDemoWindow();
        // ImGui Popup

        static bool menu_open = false; // Start with the menu closed

        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver); // Ensure it starts closed

        if (ImGui::Begin("Menu", &menu_open, ImGuiWindowFlags_NoSavedSettings)) {

            // Primitive Header
            if (ImGui::CollapsingHeader("Primitive")) {
                float speed_f = static_cast<float>(speed);
                ImGui::SliderFloat("Sphere Speed", &speed_f, 1.0f, 50.0f);
                speed = static_cast<double>(speed_f);

                if (ImGui::Button("Spawn Random Sphere")) {
                    point3 position = random_position();
                    mat material = mat(random_color());
                    auto new_sphere = make_shared<sphere>(position, 0.3, material);
                    world.add(new_sphere);

                    std::cout << "Spawned Random Sphere: " << std::endl;
                    std::cout << "Position: " << position << std::endl;
                    std::cout << "Color: " << material.diffuse_color << std::endl;
                }
            }

            //// Colors Header
            //if (ImGui::CollapsingHeader("Colors")) {
            //    // Randomize Button
            //    if (ImGui::Button("Randomize Color")) {
            //        red = random_color();
            //        mat new_material(red, 0.8, 1.0, 150.0);
            //        moving_sphere->set_material(new_material);

            //        std::cout << "Moving Sphere Color: " << new_material.diffuse_color << std::endl;
            //    }

            //    ImGui::SameLine();

            //    // Reset Button
            //    if (ImGui::Button("Reset Color")) {
            //        red = color(1.0f, 0.0f, 0.0f); // Default to red
            //        moving_sphere->set_material(mat(vec3(red[0], red[1], red[2]), 0.8, 1.0, 150.0));

            //        std::cout << "Reset Moving Sphere Color to Default Red" << std::endl;
            //    }

            //    // Color Picker for Manual Selection
            //    float color_picker_rgb[3] = { red[0], red[1], red[2] }; // Copy `red`'s current values
            //    if (ImGui::ColorEdit3("Set Sphere Color", color_picker_rgb)) {
            //        // Update `red`'s RGB values and the material
            //        red = color(color_picker_rgb[0], color_picker_rgb[1], color_picker_rgb[2]);
            //        moving_sphere->set_material(mat(vec3(red[0], red[1], red[2]), 0.8, 1.0, 150.0));

            //        std::cout << "Updated Moving Sphere Color via Picker to: "
            //            << "R: " << red[0] << ", G: " << red[1] << ", B: " << red[2] << std::endl;
            //    }
            //}

            if (ImGui::CollapsingHeader("Camera")) {
                if (ImGui::SliderFloat3("Camera Origin", camera_origin, -10.0f, 10.0f)) {
                    origin = point3(camera_origin[0], camera_origin[1], -camera_origin[2]);
                    viewport_height = 2.0 * tan((camera_fov * M_PI / 180.0) / 2);
                    viewport_width = aspect_ratio * viewport_height;
                    horizontal = vec3(viewport_width, 0, 0);
                    vertical = vec3(0, viewport_height, 0);
                    lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
                }

                // Slider para ajustar o campo de visão (FOV)
                if (ImGui::SliderFloat("Camera FOV", &camera_fov, 30.0f, 120.0f)) {
                    viewport_height = 2.0 * tan((camera_fov * M_PI / 180.0) / 2);
                    viewport_width = aspect_ratio * viewport_height;
                    horizontal = vec3(viewport_width, 0, 0);
                    vertical = vec3(0, viewport_height, 0);
                    lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
                }

                if (ImGui::Button("Reset to Default")) {
                    // Restaurar os valores padrão
                    camera_origin[0] = 0;
                    camera_origin[1] = 0;
                    camera_origin[2] = 0;
                    camera_fov = 90.0f;
                    // Recalcular parâmetros da câmera
                    origin = point3(camera_origin[0], camera_origin[1], camera_origin[2]);
                    viewport_height = 2.0 * tan((camera_fov * M_PI / 180.0) / 2);
                    viewport_width = aspect_ratio * viewport_height;
                    horizontal = vec3(viewport_width, 0, 0);
                    vertical = vec3(0, viewport_height, 0);
                    lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
                }
            }
        }

        ImGui::End();

        // Render ImGui FPS counter
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 100, 10));
        ImGui::Begin("FPS Counter", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("FPS: %.1f", fps);
        ImGui::End();

        // Render ImGui
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

        // Animate sphere position
        time += 0.01;
        point3 sphereCenter((amplitude / 2) * sin(speed * time), 0.25 - amplitude * abs(sin(speed * time)), -1);
        //moving_sphere->set_center(sphereCenter);

        // Render
#pragma omp parallel for schedule(dynamic)
        for (int l = 0; l < image_height; ++l) {
            double v = double(l) / (image_height - 1);
            vec3 vertical_component = v * vertical;
            for (int c = 0; c < image_width; ++c) {
                double u = double(c) / (image_width - 1);
                vec3 ray_direction = lower_left_corner + u * horizontal + vertical_component - origin;
                ray r(origin, ray_direction);
                color pixel_color = cast_ray(r, world, lights);
                write_color(pixels, c, l, image_width, image_height, pixel_color);
            }
        }

        // Update texture with pixel data
        SDL_UpdateTexture(texture, nullptr, pixels, image_width * sizeof(Uint32));
        // Render to window
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);

        // Render ImGui
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    delete[] pixels;
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
