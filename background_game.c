#include "background.h"
#include "background_game.h"
#include "game.h"
#include "util.h"

void setupTargetStateUnwrapped(BackgroundCycleState* target, int screenWidth)
{
    for (int i = 0; i < 3; i++) {
        // Place the left edge of the target state off-screen to the right
        target->offsets[i].x = screenWidth;
    }
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

                    game->currentCycleState->offsets[i].x = -oldX; // Segment A's position untouched
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
        // update LOD threshold and disable parallax
        game->transitionHappening = 1;
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
    } else {
        // Set to current state's color when not transitioning
        window->interpolatedSkyColor = window->cycleStates[window->currentCycleStateIndex].skyColor;
    }
}