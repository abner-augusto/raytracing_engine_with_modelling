#include "raytracer.h"

#include "light.h"
#include "sphere.h"
#include "plane.h"

#include <SDL2/SDL.h>

color phong_shading(const hit_record& rec, const vec3& view_dir, const std::vector<Light>& lights, const hittable& world) {
    // Parâmetros de luz ambiente
    double ambient_light_intensity = 0.8;  // Ajustar conforme os requisitos da cena
    double k_ambient = 0.7; // Coeficiente de reflexão ambiente para o objeto

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

        // Verificação de sombra: lança um raio de sombra do ponto de interseção em direção à luz
        ray shadow_ray(rec.p + rec.normal * 1e-3, light_dir); // Desloca ligeiramente a origem para evitar autointerseção
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

color cast_ray(const ray& r, const hittable& world, const std::vector<Light>& lights) {
    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        vec3 view_dir = unit_vector(-r.direction());
        return phong_shading(rec, view_dir, lights, world);
    }

    // Background gradient
    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.3 * (unit_direction.y() + 1.5);
    return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
}

int main(int argc, char* argv[]) {

    // Image

    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 720;

    // Calculate the image height, and ensure that it's at least 1.
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;


    // SDL Initialization
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Raytracing CG1 (atividade 3) - Abner Augusto", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, image_width, image_height, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, image_width, image_height);
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Allocate pixel buffer
    Uint32* pixels = new Uint32[image_width * image_height];

    // Definir Materiais

    color red(1.0, 0.0, 0.0);
    color green(0.0, 1.0, 0.0);
    color blue(0.0, 0.0, 1.0);
    color brown(0.69, 0.49, 0.38);

    mat sphere_mat(red, 0.8, 1.0, 150.0);
    mat sphere_mat2(green);
    mat plane_material(brown, 0.3, 0.3, 2.0);
    
    // World

    hittable_list world;

    auto moving_sphere = make_shared<sphere>(point3(0, 0.5, -1), 0.5, sphere_mat);
    world.add(moving_sphere);
    world.add(make_shared<sphere>(point3(-0.9, -0.15, -1), 0.3, sphere_mat2));
    world.add(make_shared<plane>(point3(0, -0.5, 0), vec3(0, 1, 0), plane_material));

    //Light
    std::vector<Light> lights = {
        Light(vec3(-2, 1.5, -0.2), 1.0, color(1.0, 0.5, 0.5)),
        Light(vec3(2, 2, -2), 0.7, color(0.5, 0.5, 1.0)) 
    };

    // Camera
    auto viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = 1.0;

    auto origin = point3(0, 0, 0);
    auto horizontal = vec3(viewport_width, 0, 0);
    auto vertical = vec3(0, viewport_height, 0);
    auto lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);

    // Sphere movement parameters
    double time = 0.0;
    double speed = 15.0;
    double amplitude = 0.1;

    // Render loop
    bool running = true;
    SDL_Event event;
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Animate sphere position
        time += 0.01;
        point3 sphereCenter(0, 0, -1 + amplitude * sin(speed * time));
        moving_sphere->set_center(sphereCenter);

        // Render
#pragma omp parallel for
        for (int l = 0; l < image_height; ++l) {
            for (int c = 0; c < image_width; ++c) {
                auto u = double(c) / (image_width - 1);
                auto v = double(l) / (image_height - 1);
                ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
                color pixel_color = cast_ray(r, world, lights);

                write_color(pixels, c, l, image_width, image_height, pixel_color);
            }
        }

        // Update texture with pixel data
        SDL_UpdateTexture(texture, nullptr, pixels, image_width * sizeof(Uint32));

        // Render to window
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    delete[] pixels;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
