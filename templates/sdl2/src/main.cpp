#include <SDL.h>
#include <nxdev/nxdev.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    nxdev::log::info("Initializing SDL2 for {{PROJECT_NAME}}...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        nxdev::log::error("SDL_Init failed: " + std::string(SDL_GetError()));
        return 1;
    }

    auto sdl_guard = nxdev::scope_exit([]() {
        SDL_Quit();
    });

    SDL_Window* window = SDL_CreateWindow(
        "{{PROJECT_NAME}}",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        nxdev::log::error("SDL_CreateWindow failed: " + std::string(SDL_GetError()));
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyWindow(window);
        return 1;
    }

    bool running = true;
    SDL_Event event;

    // Color animation state
    Uint8 r = 32, g = 64, b = 128;
    int step = 1;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_JOYBUTTONDOWN) {
                // Exit on Plus button (button 10 in standard SDL2 mapping for Switch)
                if (event.jbutton.button == 10) {
                    running = false;
                }
            }
        }

        // Animate background color
        r = static_cast<Uint8>(r + step);
        if (r > 200 || r < 30) step = -step;

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderClear(renderer);

        // Draw centered rectangle
        SDL_Rect rect = { 1280 / 2 - 100, 720 / 2 - 50, 200, 100 };
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}
