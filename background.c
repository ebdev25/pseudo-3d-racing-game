#include "background.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

// ====================
// Global State
// ====================
BackgroundCycleState cycleStates[3];
// ====================
// Functions
// ====================

float calculateCurvatureFactor(double roadCurvature) {
    return CURVATURE_SCALING_FACTOR * powf(fabs((float)roadCurvature), 1.5f); // Exponential sensitivity
}


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

void updateParallax(BackgroundCycleState* state, double playerSpeed,
                    double roadCurvature, double elevation, double position,
                    double previousPosition, double dt, double horizonY)
{
    for (int i = 0; i < 3; i++) {
        // X offset updates
        float parallaxSpeed = state->speeds[i] * (float)playerSpeed;
        float baseMovement  = parallaxSpeed * dt * 0.3f;
        float directionFactor = (roadCurvature >= 0.0) ? -1.0f : 1.0f;
        float directionalMovement = directionFactor * parallaxSpeed * dt * fabs(roadCurvature) * 0.5f;
        state->offsets[i].x += (baseMovement + directionalMovement);

        int textureWidth = state->textureRects[i].w;
        state->offsets[i].x = fmodf(state->offsets[i].x, (float)textureWidth);
        if (state->offsets[i].x < 0) {
            state->offsets[i].x += textureWidth;
        }

        // Y offset: dynamic horizon-based + unique margin
        float targetY = (float)horizonY - state->textureRects[i].h + state->overlapMargins[i];
        // LERP from current to target
        state->offsets[i].y = lerp_float(state->offsets[i].y, targetY, 0.1f);
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

void updateTransitionColors(SlidingWindow* window, float deltaTime) {
    if (window->transitioning) {
        // Smooth transition progress using easing
        float easedProgress = easeInOut(0.0, 1.0, window->transitionProgress);

        // Get interpolated sky color value
        const BackgroundColor* currentSkyColor = &window->cycleStates[window->currentCycleStateIndex].skyColor;
        const BackgroundColor* targetSkyColor = &window->cycleStates[window->targetCycleStateIndex].skyColor;
        window->interpolatedSkyColor = interpolateColor(currentSkyColor, targetSkyColor, easedProgress);

        // Alpha blending for current and target textures
        Uint8 currentAlpha = (Uint8)((1.0f - easedProgress) * 255);
        Uint8 targetAlpha = (Uint8)(easedProgress * 255);

        // Apply alpha blending for the layers
        for (int i = 0; i < 3; i++) {
            SDL_SetTextureAlphaMod(window->cycleStates[window->currentCycleStateIndex].texture, currentAlpha);
            SDL_SetTextureAlphaMod(window->cycleStates[window->targetCycleStateIndex].texture, targetAlpha);
        }
    }
}

void freeBackgroundResources(SlidingWindow* window) {
    if (!window) return;

    // Free textures in each cycle state
    for (int i = 0; i < window->cycleStateCount; i++) {
        BackgroundCycleState* state = &window->cycleStates[i];

        if (state->texture) {
            SDL_DestroyTexture(state->texture);
            state->texture = NULL;
        }
    }

    // Free the cycleStates array
    free(window->cycleStates);
    window->cycleStates = NULL;

    // Free the SlidingWindow itself
    free(window);
    window = NULL;
}
