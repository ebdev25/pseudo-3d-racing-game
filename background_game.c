#include "background.h"
#include "background_game.h"
#include "game.h"
#include "util.h"

void setupTargetStateUnwrapped(BackgroundCycleState* target, int screenWidth)
{
    for (int i = 0; i < 3; i++) {
        // Place the left edge of the target state off-screen to the right
        target->offsets[i].x = screenWidth;

        // Optionally reset Y offsets to match the initial vertical alignment
        //target->offsets[i].y = 0; // Or a default alignment value
    }
}

void clampLayerOffsetsToVisibleRange(Game* game) {
    if (!game->currentCycleState) {
        SDL_Log("clampLayerOffsetsToVisibleRange: No currentCycleState, skipping.\n");
        return;
    }

    int screenWidth = game->width;

    for (int i = 0; i < 3; i++) {
        int oldOffset = game->currentCycleState->offsets[i].x;
        int texW      = game->currentCycleState->textureRects[i].w;

        // For demonstration, clamp offset to [0, screenWidth - texW]
        // If screenWidth < texW, maxOffset becomes 0 => pinned left
        int minOffset = 0;
        int maxOffset = (screenWidth > texW) ? (screenWidth - texW) : 0;

        game->currentCycleState->offsets[i].x = 
            (int)limit((double)oldOffset, (double)minOffset, (double)maxOffset);

        int newOffset = game->currentCycleState->offsets[i].x;

        // DEBUG LOG
        SDL_Log("clampLayerOffsetsToVisibleRange: layer=%d oldOffset=%d newOffset=%d "
                "screenWidth=%d texW=%d minOff=%d maxOff=%d",
                i, oldOffset, newOffset, screenWidth, texW, minOffset, maxOffset);
    }
}

void updateBackgroundSlidingWindow(Game* game, SlidingWindow* window, 
                                   float deltaTime, double playerSpeed, double roadCurvature) 
{
    // Transition logic removed; transitions are now immediate
    // Parallax is disabled upon transition via game->transitionHappening
}

void startBackgroundTransition(Game* game, SlidingWindow* window, int targetIndex)
{
    if (targetIndex >= 0 && targetIndex < window->cycleStateCount)
    {
        game->transitionHappening = 1;
        window->transitioning = true;

        window->targetCycleStateIndex = targetIndex;
        BackgroundCycleState* target = &window->cycleStates[targetIndex];
        game->targetCycleState = target;

        window->transitionStartPosition = game->position;
        window->transitionProgress = 0.0; // Reset progress

        // Unwrap current
        if (game->currentCycleState) {
            for (int i = 0; i < 3; i++) {
                int texW = game->currentCycleState->textureRects[i].w;
                int oldX = game->currentCycleState->offsets[i].x;
                int moddedX = oldX % texW;
                if (moddedX < 0) moddedX += texW;

                int unwrappedOffset = -moddedX;
                int rightEdgeX = unwrappedOffset + texW; // Get position of rightmost edge of segment

                if (rightEdgeX > 0 && rightEdgeX < game->width) {
                    // Case where two visible segments (A & B) are present

                    int visibleSlice = rightEdgeX;
                    game->currentCycleState->extraSlice[i] = visibleSlice; // Store Segment B's width

                    // Preserve Segment A's position exactly!
                    game->currentCycleState->offsets[i].x = -oldX; // Do NOT modify Segment A's position
                } else {
                    // Case where only one segment is visible - standard unwrapping
                    game->currentCycleState->extraSlice[i] = 0;
                    game->currentCycleState->offsets[i].x = unwrappedOffset;
                }
            }
        }

        // Position target offscreen to the right
        if (game->targetCycleState) {
            for (int i = 0; i < 3; i++) {
                setupTargetStateUnwrapped(game->targetCycleState, game->width);
            }
        }
        SDL_Log("Transition started: old=%d, target=%d", 
                 window->currentCycleStateIndex, targetIndex);
    }
}

void advanceToNextState(Game* game, SlidingWindow* window) {
    if (window->cycleStateCount > 1) {
        int targetIndex = (window->currentCycleStateIndex + 1) % window->cycleStateCount;
        startBackgroundTransition(game, window, targetIndex);
        // Just update LOD threshold and disable parallax
        game->transitionHappening = 1;
        //game->lodCycleThreshold += (game->road.trackLength / 3.0);
        //if (game->lodCycleThreshold > game->road.trackLength) {
        //    game->lodCycleThreshold -= game->road.trackLength;
        //}
    }
}

void updateParallax(Game* game, SlidingWindow* window, BackgroundCycleState* state, double playerSpeed,
                    double roadCurvature, double elevation, double position,
                    double previousPosition, double dt, double horizonY, bool transitionHappening)
{
    // Handle special post-transition wrapping
    if (window->transitionElapsed) {
        // Apply the saved offsets from the target state
        for (int i = 0; i < 3; i++) {
            state->offsets[i].x = game->targetCycleState->offsets[i].x;
        }

        // Reset the flag after the one-time adjustment
        window->transitionElapsed = false;
        return;  // Skip the usual parallax update for this frame
    }
    for (int i = 0; i < 3; i++) {
        if (transitionHappening) {
            // Skip parallax during transition
            continue;
        }
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

void updateTransitionColors(Game* game, SlidingWindow* window, float deltaTime) {
    if (window->transitioning) {
        // Calculate progress based on actual distance moved
        window->transitionProgress = (float)((game->position - window->transitionStartPosition) / window->transitionDistance);
        window->transitionProgress = limit(window->transitionProgress, 0.0f, 1.0f);

        // Apply easing to the progress
        float easedProgress = easeInOutCubic(window->transitionProgress);

        // Interpolate sky color
        const BackgroundColor* currentSkyColor = &window->cycleStates[window->currentCycleStateIndex].skyColor;
        const BackgroundColor* targetSkyColor = &window->cycleStates[window->targetCycleStateIndex].skyColor;
        window->interpolatedSkyColor = interpolateColor(currentSkyColor, targetSkyColor, easedProgress);

        // Alpha blending for current and target textures
        Uint8 currentAlpha = (Uint8)((1.0f - easedProgress) * 255);
        Uint8 targetAlpha = (Uint8)(easedProgress * 255);

        // Apply alpha blending for the layers
        for (int i = 0; i < 3; i++) {
            //SDL_SetTextureAlphaMod(window->cycleStates[window->currentCycleStateIndex].texture, currentAlpha);
            //SDL_SetTextureAlphaMod(window->cycleStates[window->targetCycleStateIndex].texture, targetAlpha);
        }
    } else {
        // Set to current state's color when not transitioning
        window->interpolatedSkyColor = window->cycleStates[window->currentCycleStateIndex].skyColor;
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