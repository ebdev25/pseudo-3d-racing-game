#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL.h>
#include <stdbool.h>

// ====================
// Transition constants
// ====================
#define BACKGROUND_SCROLL_SCALE 0.1f
#define SPEED_SCALING_FACTOR 0.01f   // Adjusts how player speed impacts transitions
#define CURVATURE_SCALING_FACTOR 0.03f  // Adjusts how curvature affects transitions
#define DEPTH_SCALING_BASE 0.1f    // Base scaling factor for layer depth
#define MIN_LAYER_TRANSITION_SPEED 0.1f
#define MAX_LAYER_TRANSITION_SPEED 0.3f

// ====================
// Structs
// ====================

// Offset structure for texture positions
typedef struct {
    int x, y;
    int transitionX;
    float slideOffsetX;
} Offset;

// Color structure for interpolating biome-specific colors
typedef struct {
    Uint8 r, g, b, a;
} BackgroundColor;

// Background Cycle State structure
typedef struct {
    char* name;                     // Name of the cycle state
    SDL_Texture* texture;           // Single texture for all layers
    SDL_Rect textureRects[3];       // SDL_Rects for the source coordinates and dimensions
    Offset offsets[3];              // Array of offsets for parallax scrolling
    float speeds[3];                // Array of parallax speeds for the layers
    float verticalSpeeds[3];
    float layerTransitionSpeeds[3]; // Array of transition speeds for each layer
    float overlapMargins[3];
    BackgroundColor skyColor;       // Sky color for this state
    BackgroundColor roadColor;      // Road color for this state
    int extraSlice[3];              // Extra slice width to “append” when wrapping is detected
} BackgroundCycleState;

// Sliding Window structure for managing biome transitions
typedef struct {
    BackgroundCycleState* cycleStates;    // Array of cycle states
    int cycleStateCount;                  // Total number of states
    int currentCycleStateIndex;           // Index of the current state
    int targetCycleStateIndex;            // Index of the target state
    float transitionProgress;             // Progress of the transition [0.0 - 1.0]
    bool transitioning;                   // Indicates if a transition is in progress
    bool transitionElapsed;
    BackgroundColor interpolatedSkyColor; // Blended sky color during transitions
    BackgroundColor interpolatedRoadColor;// Blended road color during transitions
    double transitionStartPosition;
    double transitionDistance;
} SlidingWindow;

// ====================
// Function Prototypes
// ====================
float calculateCurvatureFactor(double roadCurvature);
void precomputeTransitionSpeeds(SlidingWindow* window, double maxSpeed, double maxCurvature);
//void updateParallax(BackgroundCycleState* state, double playerSpeed, double roadCurvature, 
//                    double elevation, double position, double previousPosition, double dt, double horizonY, bool transitionHappening);
BackgroundColor interpolateColor(const BackgroundColor* start, const BackgroundColor* end, float progress);
void freeBackgroundResources(SlidingWindow* window);

#endif // BACKGROUND_H
