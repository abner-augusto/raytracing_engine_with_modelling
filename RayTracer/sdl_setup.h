#pragma once
#include <iostream>
#include <SDL2/SDL.h>

bool InitializeSDL();
SDL_Window* CreateWindow(int width, int height, const std::string& title);
SDL_Renderer* CreateRenderer(SDL_Window* window);
SDL_Texture* CreateTexture(SDL_Renderer* renderer, int width, int height);
void Cleanup_SDL(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* texture);

bool InitializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    return true;
}

SDL_Window* CreateWindow(int width, int height, const std::string& title) {
    SDL_Window* window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
    }
    return window;
}

SDL_Renderer* CreateRenderer(SDL_Window* window) {
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
    }
    return renderer;
}

SDL_Texture* CreateTexture(SDL_Renderer* renderer, int width, int height) {
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
    }
    return texture;
}

void Cleanup_SDL(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* texture) {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void handle_event(const SDL_Event& event, bool& running, SDL_Window* window, double aspect_ratio, Camera& camera, RenderState& render_state ,float speed = 0.1f) {
    if (event.type == SDL_QUIT) {
        running = false;
    }

    if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window)) {
            running = false;
        }

        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            // Get the new window size
            int new_width = event.window.data1;
            int new_height = event.window.data2;

            // Adjust the window size to preserve the aspect ratio
            int adjusted_width = new_width;
            int adjusted_height = static_cast<int>(new_width / aspect_ratio);

            // If the adjusted height is greater than the available height, adjust by height
            if (adjusted_height > new_height) {
                adjusted_height = new_height;
                adjusted_width = static_cast<int>(new_height * aspect_ratio);
            }

            // Set the window size with the adjusted dimensions
            SDL_SetWindowSize(window, adjusted_width, adjusted_height);
        }
    }

    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            running = false;
            break;

        case SDLK_w: { // Move forward
            vec3 forward = camera.get_look_at() - camera.get_origin();
            forward = unit_vector(forward); // Normalize the forward vector
            camera.set_origin(camera.get_origin() + forward * speed);
            camera.set_look_at(camera.get_look_at() + forward * speed);
            break;
        }

        case SDLK_s: { // Move backward
            vec3 forward = camera.get_look_at() - camera.get_origin();
            forward = unit_vector(forward); // Normalize the forward vector
            camera.set_origin(camera.get_origin() - forward * speed);
            camera.set_look_at(camera.get_look_at() - forward * speed);
            break;
        }

        case SDLK_a: { // Move left
            vec3 right = cross(camera.get_up(), camera.get_look_at() - camera.get_origin());
            right = unit_vector(right); // Normalize the right vector
            camera.set_origin(camera.get_origin() + right * speed);
            camera.set_look_at(camera.get_look_at() + right * speed);
            break;
        }

        case SDLK_d: { // Move right
            vec3 right = cross(camera.get_up(), camera.get_look_at() - camera.get_origin());
            right = unit_vector(right); // Normalize the right vector
            camera.set_origin(camera.get_origin() - right * speed);
            camera.set_look_at(camera.get_look_at() - right * speed);
            break;
        }

        case SDLK_UP: { // Move look_at up
            point3 look_at = camera.get_look_at();
            camera.set_look_at(look_at + camera.get_up() * speed);
            break;
        }

        case SDLK_DOWN: { // Move look_at down
            point3 look_at = camera.get_look_at();
            camera.set_look_at(look_at - camera.get_up() * speed);
            break;
        }

        case SDLK_LEFT: { // Move look_at left
            point3 look_at = camera.get_look_at();
            camera.set_look_at(look_at - camera.get_right() * speed);
            break;
        }

        case SDLK_RIGHT: { // Move look_at right
            point3 look_at = camera.get_look_at();
            camera.set_look_at(look_at + camera.get_right() * speed);
            break;
        }

        case SDLK_q: { // Move origin and look_at up
            point3 origin = camera.get_origin();
            point3 look_at = camera.get_look_at();
            vec3 up = camera.get_up();
            camera.set_origin(origin + up * speed);
            camera.set_look_at(look_at + up * speed);
            break;
        }

        case SDLK_e: { // Move origin and look_at down
            point3 origin = camera.get_origin();
            point3 look_at = camera.get_look_at();
            vec3 up = camera.get_up();
            camera.set_origin(origin - up * speed);
            camera.set_look_at(look_at - up * speed);
            break;
        }

        case SDLK_1: // Set render mode: HighResolution
            render_state.set_mode(DefaultRender);
            break;

        case SDLK_2: // Set render mode: LowResolution
            render_state.set_mode(LowResolution);
            break;

        case SDLK_3: // Set render mode: DefaultRender
            render_state.set_mode(HighResolution);
            break;

        case SDLK_4: // Set render mode: Disabled
            render_state.set_mode(Disabled);
            break;
        }
    }

}
