// game.h
#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include "constants.h"
#include "road.h"

// HUD Structure - Currently unused
typedef struct {
    double value;
    SDL_Texture* texture;
    SDL_Rect rect;
} HUDItem;

typedef struct {
    HUDItem speed;
    HUDItem currentLapTime;
    HUDItem lastLapTime;
    HUDItem fastLapTime;
} HUD;

// Game structure
typedef struct Game {
    SDL_Renderer* renderer;
    double position;
    double speed;
    double playerX;
    double playerZ;
    double cameraDepth;
    double resolution;
    int width;
    int height;

    // Input keys
    int keyLeft;
    int keyRight;
    int keyAccelerate;
    int keyBrake;

    // Sprites
    SDL_Texture* background;
    SDL_Texture* spritesheet;

    // Road and track
    Road road;
    int rumbleLength;
    int totalCars;
    double maxSpeed;
    double accel;
    double breaking;
    double decel;
    double offRoadDecel;
    double offRoadLimit;
    double centrifugal;

    // Background LOD cycling
    int lodCycleThreshold;       // Distance threshold for LOD cycling
    int backgroundPhase;         // Current LOD phase index (0, 1, or 2)
    int nextBackgroundPhase;     // Next LOD phase during transition

    // Scrolling backgrounds
    double skyOffset;
    double hillOffset;
    double treeOffset;
    double housesOffset;
    double skySpeed;
    double hillSpeed;
    double treeSpeed;
    double housesSpeed;

    // Transition management
    bool isTransitioning;        // Flag indicating if a transition is in progress
    double transitionProgress;   // Progress of the transition (0.0 to 1.0)
    double transitionSpeed;      // Speed at which the transition progresses
    bool parallaxEnabled; // Whether or not individual parallax scrolling is enabled

    // Fixed offsets for transitions
    double fixedHillOffset;
    double fixedTreeOffset;
    double fixedHousesOffset;

    // Fog effect parameters
    bool fogEnabled;
    double fogIntensity;
    double fogTransitionSpeed;
    SDL_Color fogColor;

    // Camera and drawing
    int lanes;
    int drawDistance;
    double fogDensity;

    // Time
    double currentLapTime;
    double lastLapTime;

    // HUD
    HUD hud;

    Mix_Music* music;
} Game;

void initGame(Game* game, SDL_Renderer* renderer);
void handleEvent(Game* game, SDL_Event* event);
void updateGame(Game* game, double dt);
void renderGame(Game* game);
void cleanupGame(Game* game);

#endif // GAME_H