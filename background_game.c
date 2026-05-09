// background_game.c 
#include "background.h"
#include "background_game.h"
#include "game.h"
#include "util.h"

void setupTargetStateUnwrapped(BackgroundCycleState* target, int screenWidth)
{
    for (int i = 0; i < 3; i++) {
        // Place the left edge of the target state fully off-screen to the right
        target->offsets[i].x = screenWidth + target->textureRects[i].w;
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
        for (int i = 0; i < 3; i++) { // Reset target state transition data
            target->extraSlice[i] = 0.0f;
            // X will be set by setupTargetStateUnwrapped()
        }

        window->transitionStartPosition = game->position;
        window->transitionProgress = 0.0; // Reset progress

        // Unwrap current
        if (game->currentCycleState) {
            for (int i = 0; i < 3; i++) {
                float oldX = game->currentCycleState->offsets[i].x;
                int texW = game->currentCycleState->textureRects[i].w;
                int moddedX = ((int)oldX) % texW;
                if (moddedX < 0) moddedX += texW;

                int unwrappedOffset = -moddedX;
                int rightEdgeX = unwrappedOffset + texW; // Get position of rightmost edge of segment

                if (rightEdgeX > 0 && rightEdgeX < game->width) {
                    // Case where two visible segments (A & B) are present

                    int visibleSlice = rightEdgeX;
                    game->currentCycleState->extraSlice[i] = visibleSlice; // Store Segment B's width

                    game->currentCycleState->offsets[i].x = unwrappedOffset; // Segment A's position untouched
                } else {
                    // Case where only one segment is visible - standard unwrapping
                    game->currentCycleState->extraSlice[i] = 0;
                    game->currentCycleState->offsets[i].x = unwrappedOffset;
                }
            }
        }

        // Position target offscreen to the right
        if (game->targetCycleState) {
            setupTargetStateUnwrapped(game->targetCycleState, game->width);
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
    // Update the threshold for next transition point
    game->lodCycleThreshold += (game->road.trackLength / 3.0);
    // Wrap threshold if exceeds track length
    if (game->lodCycleThreshold > game->road.trackLength) {
        game->lodCycleThreshold -= game->road.trackLength;
    }
}

void updateParallax(Game* game, SlidingWindow* window, BackgroundCycleState* state, double playerSpeed,
                    double roadCurvature, double elevation, double position,
                    double previousPosition, double dt, double horizonY, bool transitionHappening)
{
    (void)previousPosition; // Mark as unused to silence warning
    (void)elevation;        // Currently unused in this logic block
    (void)position;         // Currently unused in this logic block
    (void)window;           // Currently unused

    // Handle special post-transition wrapping
    /*
    if (window->transitionElapsed) {
        // Apply the saved offsets from the target state
        for (int i = 0; i < 3; i++) {
            state->offsets[i].x = game->targetCycleState->offsets[i].x;
        }

        // Reset the flag after the one-time adjustment
        window->transitionElapsed = false;
        return;  // Skip the usual parallax update for this frame
    }
    */
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
        Segment* seg = findSegment(game, game->position);
        if (fabs(seg->curve) > 0.0001) {
            // Apply movement
            state->offsets[i].x += (baseMovement + directionalMovement);
        }
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
    (void)deltaTime; // Unused parameter
    
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

        // Removed calculation of unused currentAlpha/targetAlpha
    } else {
        // Set to current state's color when not transitioning
        window->interpolatedSkyColor = window->cycleStates[window->currentCycleStateIndex].skyColor;
    }
}

void resetSlidingWindow(SlidingWindow* window) {
    
    if (!window) return;
    // Pointer will be overwritten by the next loadLevel() so safe to NULL.
    window->cycleStates = NULL;
    // No states loaded yet for the next level
    window->cycleStateCount = 0;
    // Reset transition indices
    window->currentCycleStateIndex = 0;
    window->targetCycleStateIndex  = 0;
    // Reset transition progress
    window->transitionProgress = 0.0f;
    // No transition happening at this moment
    window->transitioning = false;
    window->transitionElapsed = false;
    // Reset blended colors to fully transparent (think about defauls here later)
    window->interpolatedSkyColor = (BackgroundColor){0, 0, 0, 0};
    window->interpolatedRoadColor = (BackgroundColor){0, 0, 0, 0};
    // Reset transition distance tracking
    window->transitionStartPosition = 0.0;
    window->transitionDistance = 0.0;
}

void updateBackgroundTransition(Game* game, double dt, double startPosition)
{
    if (!(game->transitionHappening && game->slidingWindow && game->currentCycleState))
        return;

    SlidingWindow* window = game->slidingWindow;

    // --- Update transition color interpolation progress ---
    double distanceIntoTransition =
        increase(game->position, -window->transitionStartPosition, game->road.trackLength);

    window->transitionProgress = limit(
        distanceIntoTransition / window->transitionDistance,
        0.0, 1.0
    );

    updateTransitionColors(game, window, dt);

    // --- Calculate how much background scroll is needed based on player movement ---
    double movementSinceLastFrame = increase(game->position, -startPosition, game->road.trackLength);
    double backgroundScrollSpeed = movementSinceLastFrame * BACKGROUND_SCROLL_SCALE;

    // --- Scroll outgoing current state layers leftwards ---
    bool allCurrentLayersOffScreen = true;

    for (int i = 0; i < 3; i++) {

        game->currentCycleState->offsets[i].x -= (float)backgroundScrollSpeed;

        int texW    = game->currentCycleState->textureRects[i].w;
        int extraW  = game->currentCycleState->extraSlice[i];
        int totalW  = texW + extraW;

        if (game->currentCycleState->offsets[i].x > -totalW)
            allCurrentLayersOffScreen = false;

        float targetY =
            (float)game->horizonY
          - game->currentCycleState->textureRects[i].h
          + game->currentCycleState->overlapMargins[i];

        game->currentCycleState->offsets[i].y =
            lerp_float(game->currentCycleState->offsets[i].y, targetY, 0.1f);
    }

    // --- Scroll incoming layers once outgoing layers are fully off-screen ---
    if (allCurrentLayersOffScreen && game->targetCycleState) {

        bool targetFullyOnScreen = true;

        for (int i = 0; i < 3; i++) {

            game->targetCycleState->offsets[i].x -= (float)backgroundScrollSpeed;

            float targetY =
                (float)game->horizonY
              - game->targetCycleState->textureRects[i].h
              + game->targetCycleState->overlapMargins[i];

            game->targetCycleState->offsets[i].y =
                lerp_float(game->targetCycleState->offsets[i].y, targetY, 0.1f);

            // Check if this layer is fully visible yet
            if (game->targetCycleState->offsets[i].x > 0)
                targetFullyOnScreen = false;
        }

        // --- Finalize transition when incoming state is fully on-screen ---
        if (targetFullyOnScreen) {

            window->currentCycleStateIndex = window->targetCycleStateIndex;
            game->currentCycleState = &window->cycleStates[window->currentCycleStateIndex];

            window->targetCycleStateIndex =
                (window->currentCycleStateIndex + 1) % window->cycleStateCount;

            if (window->cycleStateCount > 1)
                game->targetCycleState = &window->cycleStates[window->targetCycleStateIndex];
            else
                game->targetCycleState = NULL;

            // Clean up transition state before flag reset
            for (int i = 0; i < 3; i++) {
                game->currentCycleState->extraSlice[i] = 0.0f;

                // Ensure offsets are wrapped cleanly
                int texW = game->currentCycleState->textureRects[i].w;
                if (texW > 0) {
                    game->currentCycleState->offsets[i].x =
                        fmodf(game->currentCycleState->offsets[i].x, (float)texW);
                    if (game->currentCycleState->offsets[i].x < 0)
                        game->currentCycleState->offsets[i].x += texW;
                }
            }
            // End of transition state cleanup block

            // Reset transition flags
            game->transitionHappening = 0;
            window->transitioning = false;
            window->transitionProgress = 0.0f;

            SDL_Log("Background transition complete! Current state: %d\n",
                    window->currentCycleStateIndex);
        }
    }
}