#include "raytracer.h"

#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"

#include <SDL2/SDL.h>
#include <cmath>

color phong_shading(const vec3& normal, const color& obj_color) {
    // Fixed light direction and color
    vec3 light_dir = unit_vector(vec3(0.3, 0.8, -1.0)); // Light coming from the viewer's direction
    color light_color(1.0, 1.0, 1.0); // White light

    // Phong shading parameters
    double ambient_strength = 0.5;
    double diffuse_strength = 0.5;

    // Ambient component
    color ambient = ambient_strength * light_color;

    // Diffuse component
    double diff = std::max(dot(normal, light_dir), 0.0);
    color diffuse = diffuse_strength * diff * light_color;

    // Combine ambient and diffuse components
    return obj_color * (ambient + diffuse);
}

color ray_color(const ray& r, const hittable& world) {
    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        return phong_shading(rec.normal, rec.obj_color);
    }
    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
}

int main(int argc, char* argv[]) {

    // Image

    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 800;

    // Calculate the image height, and ensure that it's at least 1.
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;


    // SDL Initialization
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Raytracing CG1 (atividade 1) - Abner Augusto", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, image_width, image_height, SDL_WINDOW_SHOWN);
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

    // Sphere Colors
    color sphereColor1(1.0, 0.0, 0.0);
    color sphereColor2(0.0, 1.0, 0.0);
    //color bgColor(100.0 / 255, 100.0 / 255, 100.0 / 255);
    
    // World

    hittable_list world;

    auto moving_sphere = make_shared<sphere>(point3(0, 0, -1), 0.5, sphereColor1);
    world.add(moving_sphere);
    world.add(make_shared<sphere>(point3(0, -100.5, -1), 100, sphereColor2));

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

        // Update sphere position
        time += 0.01;
        point3 sphereCenter(0, 0, -1 + amplitude * sin(speed * time));
        moving_sphere->set_center(sphereCenter);

        // Render
        for (int l = 0; l < image_height; ++l) {
            for (int c = 0; c < image_width; ++c) {
                auto u = double(c) / (image_width - 1);
                auto v = double(l) / (image_height - 1);
                ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
                color pixel_color = ray_color(r, world);

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
