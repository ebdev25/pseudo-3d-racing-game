#include "background.h"
#include "background_game.h"
#include "game.h"
#include "util.h"

void updateBackgroundSlidingWindow(Game* game, SlidingWindow* window, 
                                   float deltaTime, double playerSpeed, double roadCurvature) 
{
    if (window->transitioning) {
        // Calculate transition speed components
        float baseSpeed = 0.1f;
        float speedFactor = SPEED_SCALING_FACTOR * (float)playerSpeed;
        float curvatureFactor = calculateCurvatureFactor(roadCurvature);

        // Total speed factor with clamping
        float totalSpeedFactor = (baseSpeed + speedFactor + curvatureFactor) * 0.5f;
        totalSpeedFactor = limit(totalSpeedFactor, MIN_LAYER_TRANSITION_SPEED, MAX_LAYER_TRANSITION_SPEED);

        // Update transition progress
        window->transitionProgress += deltaTime * totalSpeedFactor;

        // Clamp progress and finalize transition
        if (window->transitionProgress >= 1.0f) {
            window->transitionProgress = 1.0f;
            window->transitioning = false; // End the transition

            game->transitionHappening = 0;
            game->transitionElapsed   = 1;

            // Become the new current state
            window->currentCycleStateIndex = window->targetCycleStateIndex; 

            BackgroundCycleState* current = &window->cycleStates[window->currentCycleStateIndex];

            // Reset alpha for the new state to ensure full visibility
            SDL_SetTextureAlphaMod(current->texture, 255);

            // Reset targetCycleState pointer
            game->targetCycleState = NULL;
        }
    }
}

void startBackgroundTransition(Game* game, SlidingWindow* window, int targetIndex) {
    if (targetIndex >= 0 && targetIndex < window->cycleStateCount) {
        window->targetCycleStateIndex = targetIndex;
        window->transitioning = true;
        window->transitionProgress = 0.0f;

        game->transitionHappening = 1;
        game->transitionElapsed = 0;

        // Force the target's Y offsets to match the current state's Y offsets
        BackgroundCycleState* current = &window->cycleStates[window->currentCycleStateIndex];
        BackgroundCycleState* target = &window->cycleStates[targetIndex];

        for (int i = 0; i < 3; i++) {
            target->offsets[i].x = current->offsets[i].x;
            target->offsets[i].y = current->offsets[i].y;
        }

        game->targetCycleState = target;
    }
}

void advanceToNextState(Game* game, SlidingWindow* window) {
    //SDL_Log("advanceToNextState running.");
    if (window->cycleStateCount > 1) { // Ensure there is more than one state
        int nextIndex = (window->currentCycleStateIndex + 1) % window->cycleStateCount;
        startBackgroundTransition(game, window, nextIndex);
    }
}

void updateLayerTransitions(Game* game, 
                            BackgroundCycleState* currentState, 
                            BackgroundCycleState* targetState,
                            float deltaTime, double playerSpeed, double roadCurvature)
{
    if (!targetState) {
        SDL_Log("Target state null.");
        return;
    }

    for (int i = 0; i < 3; i++) {
        float depthFactor = DEPTH_SCALING_BASE * (i + 1);
        float speedFactor = SPEED_SCALING_FACTOR * (float)playerSpeed;
        float curvatureFactor = calculateCurvatureFactor(roadCurvature);

        // Horizontal slide speed
        float slideSpeed = currentState->speeds[i] * depthFactor 
                           + speedFactor 
                           + curvatureFactor;

        // Limit transition speed
        float layerTransitionSpeed = currentState->layerTransitionSpeeds[i] * depthFactor 
                                     + speedFactor 
                                     + curvatureFactor;
        layerTransitionSpeed = limit(layerTransitionSpeed, 
                                     MIN_LAYER_TRANSITION_SPEED, 
                                     MAX_LAYER_TRANSITION_SPEED);

        // Update slide offsets (X-only)
        currentState->offsets[i].slideOffsetX -= slideSpeed * deltaTime;
        targetState->offsets[i].slideOffsetX  += slideSpeed * deltaTime;

        // Clamp slide offsets if transition is fully done
        if (game->slidingWindow->transitionProgress >= 1.0f) {
            currentState->offsets[i].slideOffsetX = -currentState->textureRects[i].w;
            targetState->offsets[i].slideOffsetX  = 0.0f;
        }

        // Simple eased interpolation for X
        float easedProgress = easeInOut(0.0f, 1.0f, deltaTime);
        currentState->offsets[i].x = (float)(
            (1.0f - easedProgress) * currentState->offsets[i].x +
             easedProgress         * targetState->offsets[i].x
        );

        // Update stored X and Y from the current state's perspective
        game->storedOffsets[i]  = currentState->offsets[i].x;
        game->storedOffsetsY[i] = currentState->offsets[i].y;
    }
}