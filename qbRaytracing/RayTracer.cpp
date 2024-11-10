#include "raytracer.h"

#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"

#include <SDL2/SDL.h>
#include <cmath>

color ray_color(const ray& r, const hittable& world, const color& obj_color, const color& bgColor) {
    hit_record rec;
    if (world.hit(r, 0, infinity, rec)) {
        return obj_color;
    }

    return bgColor;
}

    // alternative background calculation:
    // vec3 unit_direction = unit_vector(r.direction());
    // auto a = 0.5 * (unit_direction.y() + 1.0);
    // return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);

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

    // World

    hittable_list world;

    world.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
    world.add(make_shared<sphere>(point3(0, -100.5, -1), 100));

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

    // Sphere and colors
    double sphereRadius = 0.5;
    color sphereColor(1.0, 0.0, 0.0);
    color bgColor(100.0 / 255, 100.0 / 255, 100.0 / 255);

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

        // Render
        for (int l = 0; l < image_height; ++l) {
            for (int c = 0; c < image_width; ++c) {
                auto u = double(c) / (image_width - 1);
                auto v = double(l) / (image_height - 1);
                ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
                color pixel_color = ray_color(r, sphereCenter, sphereColor, bgColor);

                int ir = static_cast<int>(255.999 * pixel_color.x());
                int ig = static_cast<int>(255.999 * pixel_color.y());
                int ib = static_cast<int>(255.999 * pixel_color.z());

                pixels[(image_height - 1 - l) * image_width + c] = (ir << 16) | (ig << 8) | ib;
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
