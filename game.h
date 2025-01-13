#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include "background.h"
#include "constants.h"
#include "road.h"
#include "level.h"

// HUD Item structure
typedef struct {
    double value;            // Value to display
    SDL_Texture* texture;    // Pre-rendered texture for display
    SDL_Rect rect;           // Position and size of the HUD item
} HUDItem;

// HUD structure
typedef struct {
    HUDItem speed;           // Speed display
    HUDItem currentLapTime;  // Current lap time display
    HUDItem lastLapTime;     // Last lap time display
    HUDItem fastLapTime;     // Best lap time display
} HUD;

// Game structure
typedef struct Game {
    SDL_Renderer* renderer;     // SDL renderer
    double position;            // Player's position along the track
    double speed;               // Current player speed
    double playerX;             // Player's horizontal offset from road center
    double playerZ;             // Camera position along the Z-axis
    double cameraDepth;         // Camera depth for perspective projection
    double resolution;          // Screen resolution scaling factor
    int width;                  // Screen width
    int height;                 // Screen height

    // Input keys
    int keyLeft;                // Left movement key state
    int keyRight;               // Right movement key state
    int keyAccelerate;          // Accelerate key state
    int keyBrake;               // Brake key state

    //int keyOffset;  // 0 or 1, toggled by J key

    // Sprites
    SDL_Texture* background;    // Background texture
    SDL_Texture* spritesheet;   // Spritesheet texture

    // Road and track parameters
    Road road;                  // Road structure containing segments and cars
    int rumbleLength;           // Length of rumble strips
    int totalCars;              // Total number of cars on the track
    double maxSpeed;            // Maximum player speed
    double accel;               // Acceleration rate
    double breaking;            // Braking rate
    double decel;               // Deceleration rate (natural)
    double offRoadDecel;        // Deceleration rate when off-road
    double offRoadLimit;        // Maximum speed when off-road
    double centrifugal;         // Centrifugal force for turning

    // Background LOD cycling
    int lodCycleThreshold;      // Distance threshold for LOD cycling

    // Parallax scrolling
    SlidingWindow* slidingWindow;    // Sliding window for managing LOD states
    double cycleStateWidth;          // Width in pixels of one cycle state
    BackgroundCycleState* currentCycleState; // Current cycle state
    BackgroundCycleState* targetCycleState;  // Target cycle state
    float colorProgress;             // Color blending progress during transitions

    float storedOffsets[3];
    float storedOffsetsY[3];
    int transitionHappening;
    int transitionElapsed;

    // Camera and drawing parameters
    int lanes;                  // Number of road lanes
    int drawDistance;           // Number of segments drawn ahead of the player
    double fogDensity;          // Density of the fog effect
    double horizonY;               // Y-coordinate for the horizon

    // Timing parameters
    double currentLapTime;      // Current lap time
    double lastLapTime;         // Time for the last completed lap

    // HUD
    HUD hud;                    // Heads-Up Display (HUD) structure

    // Audio
    Mix_Music* music;           // Background music
    LevelRoadData loadedRoadData;
} Game;

extern Game game;

// Function declarations
void initGame(Game* game, SDL_Renderer* renderer); // Initialize the game
void handleEvent(Game* game, SDL_Event* event);    // Handle input events
void updateGame(Game* game, double dt);            // Update game state
void renderGame(Game* game);                       // Render the game
void cleanupGame(Game* game);                      // Cleanup resources
double calculateHorizonY(Game *game);              // Calculate projected horizon line on screen

#endif // GAME_H