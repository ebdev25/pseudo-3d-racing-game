#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include "constants.h"
#include "road.h"

// HUD Item structure
typedef struct {
    double value;            // Value to display (e.g., speed, time)
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

// Transition phases enumeration
typedef enum TransitionPhase {
    TRANSITION_NONE,
    TRANSITION_ACTIVE
} TransitionPhase;

// Game structure
typedef struct Game {
    SDL_Renderer* renderer;  // SDL renderer
    double position;         // Player's position along the track
    double speed;            // Current player speed
    double playerX;          // Player's horizontal offset from road center
    double playerZ;          // Camera position along the Z-axis
    double cameraDepth;      // Camera depth for perspective projection
    double resolution;       // Screen resolution scaling factor
    int width;               // Screen width
    int height;              // Screen height

    // Input keys
    int keyLeft;             // Left movement key state
    int keyRight;            // Right movement key state
    int keyAccelerate;       // Accelerate key state
    int keyBrake;            // Brake key state

    // Sprites
    SDL_Texture* background; // Background texture
    SDL_Texture* spritesheet; // Spritesheet texture

    // Road and track parameters
    Road road;               // Road structure containing segments and cars
    int rumbleLength;        // Length of rumble strips
    int totalCars;           // Total number of cars on the track
    double maxSpeed;         // Maximum player speed
    double accel;            // Acceleration rate
    double breaking;         // Braking rate
    double decel;            // Deceleration rate (natural)
    double offRoadDecel;     // Deceleration rate when off-road
    double offRoadLimit;     // Maximum speed when off-road
    double centrifugal;      // Centrifugal force for turning

    // Background LOD cycling
    int lodCycleThreshold;   // Distance threshold for LOD cycling
    int backgroundPhase;     // Current LOD phase index (0, 1, or 2)
    int nextBackgroundPhase; // Next LOD phase during a transition

    // Scrolling backgrounds
    double skyOffset;        // Horizontal offset for sky layer
    double hillOffset;       // Horizontal offset for hills layer
    double treeOffset;       // Horizontal offset for trees layer
    double mountainOffset;   // Horizontal offset for mountains layer
    double skySpeed;         // Scrolling speed for sky layer
    double hillSpeed;        // Scrolling speed for hills layer
    double treeSpeed;        // Scrolling speed for trees layer
    double mountainSpeed;    // Scrolling speed for mountains layer

    double backgroundVerticalOffset;
    double previousPlayerY;
    double hillsParallaxFactor;
    double treesParallaxFactor;
    double mountainParallaxFactor;

    // Transition management
    TransitionPhase transitionPhase;
    bool isTransitioning;    // Indicates if a transition is in progress
    double transitionProgress; // Progress of the current transition (0.0 to 1.0)
    double transitionSpeed;  // Speed at which the transition progresses
    bool parallaxEnabled;    // Enables/disables parallax scrolling

    // Fixed offsets for transitions
    double fixedHillOffset;  // Fixed offset for hills during transitions
    double fixedTreeOffset;  // Fixed offset for trees during transitions
    double fixedMountainOffset; // Fixed offset for mountains during transitions

    // Transition specific vars
    double transitionSavedOffsetL2;
    double transitionSavedOffsetL3;
    double transitionSavedOffsetL4;

    // New fields for saturation feature
    SDL_Texture* blendedHighLODTextures[BACKGROUND_CYCLE_STATES_COUNT]; // Blended High LOD (L4) textures
    SDL_Texture* blendedMidLODTextures[BACKGROUND_CYCLE_STATES_COUNT];  // Blended Mid LOD (L3) textures
    bool saturationOn; // Flag to toggle saturation effect

    // New fields for parallax lock
    bool parallaxLock;          // Lock the parallax to current cycle state
    SlidingWindow* slidingWindow;
    double cycleStateWidth;     // Width in pixels of one cycle state

    // Fog effect parameters
    bool fogEnabled;         // Enables/disables fog effect
    double fogIntensity;     // Current fog intensity (0.0 to 1.0)
    double fogTransitionSpeed; // Speed of fog intensity transitions
    SDL_Color fogColor;      // Color of the fog effect

    // Camera and drawing parameters
    int lanes;               // Number of road lanes
    int drawDistance;        // Number of segments drawn ahead of the player
    double fogDensity;       // Density of the fog effect

    // Timing parameters
    double currentLapTime;   // Current lap time
    double lastLapTime;      // Time for the last completed lap

    // HUD
    HUD hud;                 // Heads-Up Display (HUD) structure

    // Audio
    Mix_Music* music;        // Background music
} Game;

// Function declarations
void initGame(Game* game, SDL_Renderer* renderer); // Initialize the game
void handleEvent(Game* game, SDL_Event* event);    // Handle input events
void updateGame(Game* game, double dt);            // Update game state
void renderGame(Game* game);                       // Render the game
void cleanupGame(Game* game);                      // Cleanup resources

#endif // GAME_H