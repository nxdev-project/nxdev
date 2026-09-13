#include <nxdev/app.hpp>
#include <nxdev/log.hpp>
#include <SDL2/SDL.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    nxdev::App app;
    nxdev::log::info("Starting SDL2 Demo...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        nxdev::log::error(std::string("SDL_Init failed: ") + SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "NXDev SDL2 Demo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        nxdev::log::error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        nxdev::log::error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    nxdev::log::info("SDL2 Initialized successfully. Rendering frame...");

    SDL_SetRenderDrawColor(renderer, 20, 24, 35, 255);
    SDL_RenderClear(renderer);

    SDL_Rect rect{540, 260, 200, 200};
    SDL_SetRenderDrawColor(renderer, 0, 180, 255, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_RenderPresent(renderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    nxdev::log::info("SDL2 Demo completed cleanly.");
    return 0;
}
