// main.c
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "game.h"

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    // Initialize SDL with video, timer, and audio
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("Mix_OpenAudio Error: %s", Mix_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("SDL Racer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1,
                              SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    printf("Renderer name: %s\n", info.name);
    printf("Renderer flags: %d\n", info.flags);

    if (info.flags & SDL_RENDERER_SOFTWARE) printf("Renderer supports software rendering.\n");
    if (info.flags & SDL_RENDERER_ACCELERATED) printf("Renderer supports hardware acceleration.\n");
    if (info.flags & SDL_RENDERER_PRESENTVSYNC) printf("Renderer supports vsync.\n");
    if (info.flags & SDL_RENDERER_TARGETTEXTURE) printf("Renderer supports rendering to texture.\n");

    Game game;
    initGame(&game, renderer);

    Uint32 lastTime = SDL_GetTicks();
    double dt = 0;

    int running = 1;
    SDL_Event e;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        dt = (currentTime - lastTime) / 1000.0;
        lastTime = currentTime;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = 0;
            handleEvent(&game, &e);
        }

        updateGame(&game, dt);
        renderGame(&game);

        SDL_RenderPresent(renderer);
    }

    cleanupGame(&game);

    // Clean up SDL_mixer
    Mix_CloseAudio();

    // Clean up SDL_ttf
    TTF_Quit();

    // Clean up SDL_image
    IMG_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}