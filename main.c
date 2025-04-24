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
                              0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
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

    if (renderer) {
        SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);
    }


    Game game;
    // Game struct field initialization
    memset(&game, 0, sizeof(Game));
    initGame(&game, renderer);

    Uint32 lastTime = SDL_GetTicks();
    double dt = 0;

    int running = 1; // Controls the main loop
    SDL_Event e;

    // Main Game Loop
    while (running) {
        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        // Prevent large dt spikes if debugging/pausing
        Uint32 deltaTicks = currentTime - lastTime;
        if (deltaTicks > 100) { // Clamp dt to avoid large jumps (max 100ms)
            deltaTicks = 100;
        }
        dt = deltaTicks / 1000.0;
        lastTime = currentTime;

        // Event Processing Loop
        while (SDL_PollEvent(&e)) {
            // Handle window close button
            if (e.type == SDL_QUIT) {
                running = 0;
                break; // Exit SDL_PollEvent loop
            }

            // Call handleEvent for the current event 'e'
            int continueRunning = handleEvent(&game, &e); // Process event

            // Check if handleEvent signaled to quit
            if (continueRunning == 0) {
                running = 0; // Set flag to stop the main loop
                break;       // Exit SDL_PollEvent loop
            }
        }

        // If running is set to 0 by event handling, exit the main loop
        if (!running) {
            break;
        }

        // Update and Render
        updateGame(&game, dt);
        renderGame(&game);

        // Present the frame
        SDL_RenderPresent(renderer);

    } // End Main Game Loop

    // Cleanup
    cleanupGame(&game);

    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}