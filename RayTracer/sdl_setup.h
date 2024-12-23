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

void handle_event(const SDL_Event& event, bool& running, SDL_Window* window, double aspect_ratio) {
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
}
