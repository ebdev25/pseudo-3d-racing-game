#include "background.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

// ====================
// Functions
// ====================


void precomputeTransitionSpeeds(SlidingWindow* window, double maxSpeed, double maxCurvature) {
    for (int stateIndex = 0; stateIndex < window->cycleStateCount; stateIndex++) {
        BackgroundCycleState* state = &window->cycleStates[stateIndex];
        for (int i = 0; i < 3; i++) { // Iterate over layers
            float depthFactor = DEPTH_SCALING_BASE * (i + 1); // Scale based on layer depth
            state->layerTransitionSpeeds[i] = depthFactor * SPEED_SCALING_FACTOR * maxSpeed + 
                                              depthFactor * CURVATURE_SCALING_FACTOR * maxCurvature;
        }
    }
}

BackgroundColor interpolateColor(const BackgroundColor* start, const BackgroundColor* end, float progress) {
    return (BackgroundColor){
        .r = (Uint8)limit((double)((1.0f - progress) * start->r + progress * end->r), 0, 255),
        .g = (Uint8)limit((double)((1.0f - progress) * start->g + progress * end->g), 0, 255),
        .b = (Uint8)limit((double)((1.0f - progress) * start->b + progress * end->b), 0, 255),
        .a = 255
    };
}

/*
 * Clear non-owning texture pointers on cycle states. The Game owns the SDL_Texture
 * (game->background); destroy that from game teardown, not here.
 */
void freeBackgroundResources(SlidingWindow* window) {
    if (!window || !window->cycleStates) {
        return;
    }
    for (int i = 0; i < window->cycleStateCount; i++) {
        window->cycleStates[i].texture = NULL;
    }
}
