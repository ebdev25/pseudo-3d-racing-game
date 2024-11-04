// game.h
#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "constants.h"
#include "road.h"

typedef struct {
    SDL_Texture* texture;
    double parallaxSpeed;
    // blendStartDistance and blendEndDistance represent the range at which the (background) segment
    // becomes visible
    double blendStartDistance;
    double blendEndDistance;
} BackgroundSegment;

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

    // Background
    BackgroundSegment backgroundSegments[MAX_BACKGROUND_SEGMENTS];

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

    // Scrolling backgrounds
    double skyOffset;
    double hillOffset;
    double treeOffset;
    double skySpeed;
    double hillSpeed;
    double treeSpeed;

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
void initBackgroundSegments(Game* game);

#endif // GAME_H