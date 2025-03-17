// game.c
#include "game.h"
#include "background_game.h"
#include "background.h"
#include "util.h"
#include "render.h"
#include "constants.h"
#include "level_loader.h"
#include "ai.h"
#include "collision.h"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

// Forward declarations
void updateCars(Game* game, double dt, Segment* playerSegment, double playerW);
double updateCarOffset(Game* game, Car* car, Segment* carSegment, Segment* playerSegment, double playerW);
int compareLeaderboardEntries(const void* a, const void* b);
// Racing lights update func
static void updateTrafficLightAnimation(Game* game, double dt)
{
    Animation* anim = &game->trafficLightAnim;
    if (!anim->active || anim->finished) {
        return; // no update needed
    }

    // Advance timer
    anim->currentTime += dt;
    double frameDuration = anim->frameDurations[anim->currentFrame];

    // Check if time to go to next frame
    if (anim->currentTime >= frameDuration) {
        anim->currentTime -= frameDuration;
        anim->currentFrame++;

        // If we moved beyond last frame
        if (anim->currentFrame >= anim->frameCount) {
            anim->currentFrame = anim->frameCount - 1; // clamp to final
            anim->finished = true;
            // Once green => race start:
            game->raceState = RACE_STATE_RUNNING;
            game->movementAllowed = true;
            return;
        }
    }
}

// Initialize the game
void initGame(Game* game, SDL_Renderer* renderer) {
    game->raceState = RACE_STATE_COUNTDOWN;
    game->movementAllowed = false; 
    // Initialize SDL_ttf for text rendering in the HUD
    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        exit(1);
    }

    // Initialize SDL_mixer for audio
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("Mix_OpenAudio Error: %s", Mix_GetError());
        TTF_Quit();
        exit(1);
    }

    // Set up renderer and screen dimensions
    game->renderer = renderer;
    game->width = WIDTH;  // Screen width
    game->height = HEIGHT; // Screen height
    game->resolution = (double)game->height / 480.0;  // Scaling factor based on screen height

    // Initialize player and camera state
    game->position = 0.0;  // Current position along the track
    game->speed = 0.0;     // Initial speed of the player
    game->playerX = 0.0;   // Horizontal offset of the player from the road center
    game->playerZ = CAMERA_HEIGHT * (1.0 / tan((FIELD_OF_VIEW / 2.0) * M_PI / 180.0));  // Distance from camera to player
    game->cameraDepth = 1.0 / tan((FIELD_OF_VIEW / 2.0) * M_PI / 180.0);  // Depth of camera view

    // Initialize input key states
    game->keyLeft = 0;
    game->keyRight = 0;
    game->keyAccelerate = 0;
    game->keyBrake = 0;

    // Initialize road and track parameters
    memset(&game->road, 0, sizeof(Road));  // Clear road struct

    // Set various track properties
    game->rumbleLength = RUMBLE_LENGTH;  // Length of alternating rumble strip colors
    game->totalCars = 200;               // Total number of cars on the track
    game->maxSpeed = MAX_SPEED;          // Maximum player speed
    game->accel = game->maxSpeed / 5.0;  // Acceleration rate
    game->breaking = -game->maxSpeed;    // Deceleration (braking) rate
    game->decel = -game->maxSpeed / 5.0; // Natural deceleration when not accelerating
    game->offRoadDecel = -game->maxSpeed / 2.0;  // Deceleration when off-road
    game->offRoadLimit = game->maxSpeed / 4.0;   // Speed limit when off-road
    game->centrifugal = 0.3;  // Centrifugal force for turning

    // Camera and drawing settings
    game->lanes = LANE_COUNT;              // Number of lanes on the road
    game->drawDistance = DRAW_DISTANCE;    // Number of segments to draw ahead of the player
    game->fogDensity = FOG_DENSITY;        // Density of fog effect for distant segments

    // Initialize lap timing variables
    game->currentLapTime = 0.0;  // Time for the current lap
    game->lastLapTime = 0.0;     // Time for the previous lap

    // Seed the random number generator for randomized elements
    srand((unsigned int)time(NULL));

    // Sliding Window init to NULL
    game->slidingWindow = NULL;

    // Load the level and associated resources
    if (!loadLevel(game, "/home/horatiobelter/Desktop/Programming/Pseudo 3D Racer/levels/beach_test.json")) {
        SDL_Log("Failed to load level.\n");
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

    // Load animated sprites
    SDL_Texture* animTex = IMG_LoadTexture(renderer, "../resources/animated_sprites.png");
    if (!animTex) {
        SDL_Log("Failed to load animated sprites: %s", IMG_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }
    game->animatedSpritesheet = animTex;

    game->trafficLightSound = Mix_LoadMUS("../resources/racing_lights_sound_longer.mp3");
    if (!game->trafficLightSound) {
        SDL_Log("Failed to load traffic light sound: %s", Mix_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

    if (Mix_PlayMusic(game->trafficLightSound, 0) == -1) {
        SDL_Log("Failed to play traffic light sound: %s", Mix_GetError());
        Mix_FreeMusic(game->trafficLightSound);
        game->trafficLightSound = NULL;
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

    // Load HUD font
    game->hudFont = TTF_OpenFont("../resources/Pro Racing.ttf", 24);
    if (!game->hudFont) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        TTF_Quit();
        exit(1);
    }

    // Initialize totalRaceTime
    game->totalRaceTime = 0.0;

    // Initialize the Animation fields
    game->trafficLightAnim.frameCount = 5;
    game->trafficLightAnim.frames = (const Sprite**)malloc(sizeof(Sprite*) * 5);
    game->trafficLightAnim.frameDurations = (double*)malloc(sizeof(double) * 5);

    // Attach the 5 frames
    game->trafficLightAnim.frames[0] = &SPRITE_LIGHT_FRAME1; 
    game->trafficLightAnim.frames[1] = &SPRITE_LIGHT_FRAME2;
    game->trafficLightAnim.frames[2] = &SPRITE_LIGHT_FRAME3;
    game->trafficLightAnim.frames[3] = &SPRITE_LIGHT_FRAME4;
    game->trafficLightAnim.frames[4] = &SPRITE_LIGHT_FRAME5;

    // Each frame ~0.92s => total 4.6s
    for (int i = 0; i < 5; i++) {
        game->trafficLightAnim.frameDurations[i] = 0.92;
    }

    game->trafficLightAnim.currentFrame = 0;
    game->trafficLightAnim.currentTime  = 0.0;
    game->trafficLightAnim.active       = true;   // start active
    game->trafficLightAnim.finished     = false;
    game->trafficLightAnim.loop         = false;  // play once

    // Load the *opponent* spritesheet
    SDL_Texture* oppTex = IMG_LoadTexture(renderer, "/home/horatiobelter/Desktop/Programming/Pseudo 3D Racer/resources/opponents_sheet_v1.png");
    if (!oppTex) {
        SDL_Log("Failed to load opponent sprites: %s", IMG_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }
    game->opponentSpritesheet = oppTex;

    // Reset road segments
    resetRoad(game);

    // Initialize additional game state variables
    game->lodCycleThreshold = (game->road.trackLength > 0.0) ? (game->road.trackLength / 3.0) : 1000.0;
    game->horizonY = 0.0;
    game->colorProgress = 0.0f;

    if (game->slidingWindow && game->slidingWindow->cycleStateCount > 0) {
        for (int i = 0; i < 3; i++) {
            game->storedOffsets[i] = game->slidingWindow->cycleStates[0].offsets[i].x;
            game->storedOffsetsY[i] = game->slidingWindow->cycleStates[0].offsets[i].y;
        }
    } else {
        memset(game->storedOffsets, 0, sizeof(game->storedOffsets));
        memset(game->storedOffsetsY, 0, sizeof(game->storedOffsetsY));
    }

    game->transitionElapsed = 0;
    game->transitionHappening = 0;

    game->slidingWindow->transitionStartPosition = 0.0;
    game->slidingWindow->transitionDistance = game->road.trackLength / 16.0; // Revisit later

    // Initialize AI Controller with 1 opponent for demonstration
    initAIController(&game->aiController, game, 3);

    // Init AI driver sprite to STRAIGHT version
    if (game->aiController.driverCount > 0) {
        game->aiController.drivers[0].sprite = &SPRITE_AI_STRAIGHT;
    }

    // Race win lose condition tracking
    game->totalLaps = 1;
    game->playerLapsCompleted = 0;
    game->raceFinished = false;

    // Collision flag
    game->inCollisionPush = false;

    double horizonY = calculateHorizonY(game);
    for (int stateIndex = 0; stateIndex < game->slidingWindow->cycleStateCount; stateIndex++) {
        BackgroundCycleState *state = &game->slidingWindow->cycleStates[stateIndex];
        // For each of the three layers...
        for (int i = 0; i < 3; i++) {
            // Compute the ideal y offset for this layer.
            float idealY = (float)horizonY - state->textureRects[i].h + state->overlapMargins[i];
            state->offsets[i].y = idealY;
        }
    }
}

void handleEvent(Game* game, SDL_Event* event) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case KEY_LEFT:
            case KEY_A:
                game->keyLeft = 1;
                break;
            case KEY_RIGHT:
            case KEY_D:
                game->keyRight = 1;
                break;
            case KEY_UP:
            case KEY_W:
                game->keyAccelerate = 1;
                break;
            case KEY_DOWN:
            case KEY_S:
                game->keyBrake = 1;
                break;
            default:
                break;
        }
    } else if (event->type == SDL_KEYUP) {
        switch (event->key.keysym.sym) {
            case KEY_LEFT:
            case KEY_A:
                game->keyLeft = 0;
                break;
            case KEY_RIGHT:
            case KEY_D:
                game->keyRight = 0;
                break;
            case KEY_UP:
            case KEY_W:
                game->keyAccelerate = 0;
                break;
            case KEY_DOWN:
            case KEY_S:
                game->keyBrake = 0;
                break;
            default:
                break;
        }
    }
}

// Update the game state
void updateGame(Game* game, double dt) {
    updateTrafficLightAnimation(game, dt);

    if (game->movementAllowed) {
        // Handle main game logic
        double playerW = SPRITE_PLAYER_STRAIGHT.w * SPRITE_SCALE;

        // Percentage of the maximum speed that the player is currently moving
        double speedPercent = (game->maxSpeed > 0.0) ? (game->speed / game->maxSpeed) : 0.0; // Avoid division by zero

        // Horizontal movement increment based on time step and speed
        double dx = dt * 2.0 * speedPercent;

        // Store the starting position to calculate distance moved
        double startPosition = game->position;

        game->totalRaceTime += dt; // Update race time

        // Find the current player segment
        Segment* playerSegment = findSegment(game, game->position + game->playerZ);
        if (playerSegment == NULL) {
            SDL_Log("Error: Player segment not found in updateGame.\n");
            return;
        }

        // Update cars on the track
        updateCars(game, dt, playerSegment, playerW);

        // Update AI opponents
        updateAIController(&game->aiController, dt);

        checkAIDriverCollisions(game);

        // Check for collisions with AI opponents
        checkPlayerAIOpponentCollisions(game);

        // Process AI ramming effects on the player.
        {
            // Check each AI driver to see if they are ramming and overlapping with the player.
            Segment* seg = findSegment(game, game->position + game->playerZ);
            if (seg) {
                for (int i = 0; i < game->aiController.driverCount; i++) {
                    AIDriver* driver = &game->aiController.drivers[i];
                    Segment* aiSeg = findSegment(game, driver->z);
                    if (aiSeg != seg)
                        continue;
                    double aiW = (driver->sprite) ? driver->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;
                    double playerLeft = game->playerX - playerW / 2;
                    double playerRight = game->playerX + playerW / 2;
                    double aiLeft = driver->offset - aiW / 2;
                    double aiRight = driver->offset + aiW / 2;
                    if ((playerRight > aiLeft && playerLeft < aiRight) && driver->isRamming) {
                        // Apply a push force to the player.
                        double pushForce = 0.1; // Adjustable force factor (later test scaling with dt here)
                        if (driver->offset < game->playerX)
                            game->playerX += pushForce;
                        else
                            game->playerX -= pushForce;
                    }
                }
            }
        }

        // Move the player's position forward based on current speed
        game->position = increase(game->position, dt * game->speed, game->road.trackLength);

        // Handle player input for left/right movement
        if (!game->inCollisionPush) {
            if (game->keyLeft)
                game->playerX -= dx;
            if (game->keyRight)
                game->playerX += dx;
        }

        // Adjust position based on the road's curve and centrifugal force
        game->playerX -= dx * speedPercent * playerSegment->curve * game->centrifugal;

        // Handle acceleration and deceleration based on input
        if (game->keyAccelerate)
            game->speed = accelerate(game->speed, game->accel, dt);
        else if (game->keyBrake)
            game->speed = accelerate(game->speed, game->breaking, dt);
        else
            game->speed = accelerate(game->speed, game->decel, dt);

        // Handle player going off-road
        if ((game->playerX < -1.0) || (game->playerX > 1.0)) {
            // Slow down player if off-road
            if (game->speed > game->offRoadLimit)
                game->speed = accelerate(game->speed, game->offRoadDecel, dt);

            // Check for collision with roadside sprites
            for (int n = 0; n < playerSegment->spriteCount; n++) {
                SpriteInstance* sprite = &playerSegment->sprites[n];
                double spriteW = sprite->source->w * SPRITE_SCALE;
                double spriteCenterOffset = sprite->offset + ((sprite->offset > 0.0) ? (spriteW / 2.0) : (-spriteW / 2.0));

                if (overlap(game->playerX, playerW, spriteCenterOffset, spriteW, 1.0)) {
                    game->speed = game->maxSpeed / 5.0;
                    game->position = increase(playerSegment->p1.world.z, -game->playerZ, game->road.trackLength);
                    SDL_Log("Collision with roadside sprite detected. Speed reduced and position reset.\n");
                    break;
                }
            }
        }

        // Check for collisions with other cars
        /*
        for (int n = 0; n < playerSegment->carCount; n++) {
            Car* car = playerSegment->cars[n];
            double carW = car->sprite->w * SPRITE_SCALE;

            if (game->speed > car->speed) {
                if (overlap(game->playerX, playerW, car->offset, carW, 0.8)) {
                    game->speed = (game->speed > 0.0) ? (car->speed * (car->speed / game->speed)) : 0.0;
                    game->position = increase(car->z, -game->playerZ, game->road.trackLength);
                    SDL_Log("Collision with car detected. Speed adjusted and position reset.\n");
                    break;
                }
            }
        }
        */

        // Limit the player’s horizontal position and speed
        game->playerX = limit(game->playerX, -3.0, 3.0);
        game->speed = limit(game->speed, 0.0, game->maxSpeed);

        // Update lap timing variables
        if (game->position > game->playerZ) {
            if (game->currentLapTime > 0.0 && (startPosition < game->playerZ)) {
                // Increment player's lap count
                game->playerLapsCompleted++;
                game->lastLapTime = game->currentLapTime;
                game->currentLapTime = 0.0;
                SDL_Log("Player completed lap #%d in %.2f seconds",
                        game->playerLapsCompleted, game->lastLapTime);
                // If the player has reached or exceeded total laps => finish
                if (game->playerLapsCompleted >= game->totalLaps) {
                    SDL_Log("Player has finished the race!");
                    // End race
                    game->raceState       = RACE_STATE_FINISHED;
                    game->movementAllowed = false;
                    game->raceFinished    = true;
                    generateLeaderboard(game);
                }
                // Reset lodCycleThreshold to the start of the new lap
                game->lodCycleThreshold -= game->road.trackLength;
                if (game->lodCycleThreshold < 0.0) {
                    game->lodCycleThreshold += game->road.trackLength;
                }
            } else {
                game->currentLapTime += dt;
            }
        }

        // Calculate projected horizon y value
        game->horizonY = calculateHorizonY(game);

        // Update parallax only if no transition has occurred
        if (!game->transitionHappening) {
            double elevation = calculateElevation(playerSegment, game->position);
            updateParallax(game, game->slidingWindow, game->currentCycleState, game->speed, playerSegment->curve, elevation, 
                           game->position, game->position - (game->speed * dt), dt, game->horizonY, game->transitionHappening);
        }

        // Automatically trigger parallax disable when passing threshold
        if (!game->slidingWindow->transitioning && game->position > game->lodCycleThreshold) {
            advanceToNextState(game, game->slidingWindow);  // This now only sets flags
            game->lodCycleThreshold += (game->road.trackLength / 3.0);
            if (game->lodCycleThreshold > game->road.trackLength) {
                game->lodCycleThreshold -= game->road.trackLength;
            }
        }

        // Move the current state's layers left by 5 pixels per frame during transition
        if (game->transitionHappening && game->currentCycleState) {
            // Transition progress, used for color interpolation
            game->slidingWindow->transitionProgress += dt / game->slidingWindow->transitionDistance;
            game->slidingWindow->transitionProgress = limit(game->slidingWindow->transitionProgress, 0.0, 1.0);

            updateTransitionColors(game, game->slidingWindow, dt);

            double movementSinceLastFrame = dt * game->speed; // Distance moved in the game world
            double backgroundScrollSpeed = movementSinceLastFrame * BACKGROUND_SCROLL_SCALE; // Convert world movement to pixels
            // 1. Scroll the current state first
            bool allLayersOffScreen = true; // we'll check all 3 layers
            for (int i = 0; i < 3; i++) {
                // Move current layer left
                game->currentCycleState->offsets[i].x -= (int)backgroundScrollSpeed;

                // Check if this layer is fully off-screen
                int texW = game->currentCycleState->textureRects[i].w;
                int extraW = game->currentCycleState->extraSlice[i];

                // If segment C exists from addition of Seg A and B, compute effective width
                int effectWidth = texW + extraW;

                if (extraW > 0) {
                    // Case where extra width exists, thus Seg A and B both on screen and need to be scrolled together
                    if (game->currentCycleState->offsets[i].x > -effectWidth) {
                        allLayersOffScreen = false;
                    }
                } else {
                    // Case where extra width doesn't exist, scroll normally
                    if (game->currentCycleState->offsets[i].x > -texW) {
                        allLayersOffScreen = false;
                    }
                }
            }

            // Compute the target horizon value (e.g. based on highestRoadY)
            double currentHorizon = calculateHorizonY(game);
            for (int i = 0; i < 3; i++) {
                // Compute the desired Y for layer i
                float targetY = (float)currentHorizon 
                                - game->currentCycleState->textureRects[i].h 
                                + game->currentCycleState->overlapMargins[i];
                // LERP toward the target Y
                game->currentCycleState->offsets[i].y =
                    lerp_float(game->currentCycleState->offsets[i].y, targetY, 0.1f);
            }

            // 2. If the current state is off-screen, start scrolling the target state
            if (allLayersOffScreen && game->targetCycleState) {
                for (int i = 0; i < 3; i++) {
                    float targetY = (float)currentHorizon 
                                - game->targetCycleState->textureRects[i].h 
                                + game->targetCycleState->overlapMargins[i];
                    game->targetCycleState->offsets[i].x -= (int)backgroundScrollSpeed; // move it left
                    game->targetCycleState->offsets[i].y = lerp_float(game->targetCycleState->offsets[i].y, targetY, 0.1f);
                }

                // NEW: Check if the target is "far enough" in to finalize
                // Let's define a "final offset" like -0
                const int SCROLL_IN_LIMIT = 0;
                bool targetFullyIn = true;
                for (int i = 0; i < 3; i++) {
                    // We'll say "layer's left edge <= SCROLL_IN_LIMIT means fully in"
                    int leftEdge = game->targetCycleState->offsets[i].x;
                    if (leftEdge > SCROLL_IN_LIMIT) {
                        targetFullyIn = false;
                        break;
                    }
                }

                // If fully in, finalize the transition
                if (targetFullyIn) {
                    // Switch the currentCycleStateIndex to the target
                    game->slidingWindow->currentCycleStateIndex = game->slidingWindow->targetCycleStateIndex;
                    game->currentCycleState =
                        &game->slidingWindow->cycleStates[game->slidingWindow->currentCycleStateIndex];

                    // Clear flags => transition done
                    game->transitionHappening = 0;
                    game->slidingWindow->transitioning = false;
                    game->slidingWindow->transitionElapsed = true;

                    SDL_Log("Transition complete! Now using state index=%d\n",
                             game->slidingWindow->currentCycleStateIndex);

                    // (Optional) clamp offsets to a valid range or re-wrap them
                    // if you'd like them to “snap” back into standard wrapping range:
                    // clampLayerOffsetsToVisibleRange(game);
                }
            }
        }

        //if (!game->slidingWindow->transitioning) {
        //    game->currentCycleState = &game->slidingWindow->cycleStates[game->slidingWindow->currentCycleStateIndex];
        //}    
        // Update HUD elements (TODO)
    }
}

// Render the game
void renderGame(Game* game) {
    // Call the render function
    render(game);

    if (game->raceState == RACE_STATE_FINISHED && game->leaderboard.isActive) {
        if (game->leaderboard.texture) {
            SDL_RenderCopy(game->renderer, game->leaderboard.texture, NULL, &game->leaderboard.rect);
        }
    }
}

// Clean up game resources
void cleanupGame(Game* game) {
    // Free resources
    if (game->background)
        SDL_DestroyTexture(game->background);
    if (game->spritesheet)
        SDL_DestroyTexture(game->spritesheet);
    if (game->opponentSpritesheet) {
        SDL_DestroyTexture(game->opponentSpritesheet);
        game->opponentSpritesheet = NULL;
    }
    if (game->animatedSpritesheet) {
        SDL_DestroyTexture(game->animatedSpritesheet);
        game->animatedSpritesheet = NULL;
    }
    // Free road segments
    if (game->road.segments) {
        for (int i = 0; i < game->road.segmentCount; i++) {
            Segment* segment = &game->road.segments[i];
            if (segment->sprites)
                free(segment->sprites);
            if (segment->cars)
                free(segment->cars);
        }
        free(game->road.segments);
    }

    // Free cars
    if (game->road.cars)
        free(game->road.cars);

    // Free SlidingWindow
    if (game->slidingWindow) {
        free(game->slidingWindow);
        game->slidingWindow = NULL;
    }

    // Free music
    if (game->music)
        Mix_FreeMusic(game->music);

    // Free AI driver array
    if (game->aiController.drivers) {
        free(game->aiController.drivers);
        game->aiController.drivers = NULL;
    }

    if (game->trafficLightSound) {
        Mix_FreeMusic(game->trafficLightSound);
        game->trafficLightSound = NULL;
    }

    if (game->trafficLightAnim.frames) {
        free(game->trafficLightAnim.frames);
        game->trafficLightAnim.frames = NULL;
    }
    if (game->trafficLightAnim.frameDurations) {
        free(game->trafficLightAnim.frameDurations);
        game->trafficLightAnim.frameDurations = NULL;
    }

        if (game->leaderboard.entries) {
        free(game->leaderboard.entries);
        game->leaderboard.entries = NULL;
    }
    if (game->leaderboard.texture) {
        SDL_DestroyTexture(game->leaderboard.texture);
        game->leaderboard.texture = NULL;
    }
    if (game->hudFont) {
        TTF_CloseFont(game->hudFont);
        game->hudFont = NULL;
    }

    // Quit SDL_ttf and SDL_mixer
    TTF_Quit();
    Mix_CloseAudio();
}

// Update AI cars on the track
void updateCars(Game* game, double dt, Segment* playerSegment, double playerW) {
    // Loop through each car on the road
    for (int n = 0; n < game->road.carCount; n++) {
        Car* car = &game->road.cars[n];

        // Find the segment the car is currently in
        Segment* oldSegment = findSegment(game, car->z);
        if (oldSegment == NULL) {
            SDL_Log("Error: Car's current segment not found.\n");
            continue;
        }

        // Adjust the car's lateral position based on player position and obstacles
        double offset = updateCarOffset(game, car, oldSegment, playerSegment, playerW);
        car->offset += offset;

        // Move the car forward based on its speed and elapsed time
        car->z = increase(car->z, dt * car->speed, game->road.trackLength);

        // Calculate car's position as a percentage of the current segment
        car->percent = percentRemaining(car->z, SEGMENT_LENGTH);

        // Find the segment the car has moved into
        Segment* newSegment = findSegment(game, car->z);
        if (newSegment == NULL) {
            SDL_Log("Error: Car's new segment not found.\n");
            continue;
        }

        // Check if the car has moved to a different segment
        if (oldSegment != newSegment) {
            // Remove car from the old segment's car list
            bool found = false;
            for (int i = 0; i < oldSegment->carCount; i++) {
                if (oldSegment->cars[i] == car) {
                    // Shift remaining cars to fill the gap
                    for (int j = i; j < oldSegment->carCount - 1; j++) {
                        oldSegment->cars[j] = oldSegment->cars[j + 1];
                    }
                    oldSegment->carCount--;
                    found = true;
                    break;
                }
            }
            if (!found) {
                SDL_Log("Warning: Car not found in old segment's car list.\n");
            }

            // Add car to the new segment's car list
            Car** resizedCars = realloc(newSegment->cars, sizeof(Car*) * (newSegment->carCount + 1));
            if (resizedCars == NULL) {
                SDL_Log("Error: realloc failed while adding car to new segment.\n");
                continue;
            }
            newSegment->cars = resizedCars;
            newSegment->cars[newSegment->carCount] = car;
            newSegment->carCount++;
        }
    }
}

// Update the lateral offset of a car based on potential collisions
double updateCarOffset(Game* game, Car* car, Segment* carSegment, Segment* playerSegment, double playerW) {
    const int lookahead = 20; // Number of segments to look ahead to avoid collisions
    double carW = car->sprite->w * SPRITE_SCALE; // Width of the car in game units

    // If the car is too far from the player, skip adjusting its offset
    if ((carSegment->index - playerSegment->index) > game->drawDistance)
        return 0.0;

    // Check each segment in the lookahead distance for potential obstacles
    for (int i = 1; i < lookahead; i++) {
        Segment* segment = &game->road.segments[(carSegment->index + i) % game->road.segmentCount];

        // Avoid the player if they are in the segment and in front of the car
        if ((segment == playerSegment) && (car->speed > game->speed) &&
            overlap(game->playerX, playerW, car->offset, carW, 1.2)) {

            double dir;
            // Determine which direction to steer based on player's position
            if (game->playerX > 0.5)
                dir = -1.0;  // Player is on the right; steer left
            else if (game->playerX < -0.5)
                dir = 1.0;   // Player is on the left; steer right
            else
                dir = (car->offset > game->playerX) ? 1.0 : -1.0; // Choose direction based on relative position

            // Return calculated steering adjustment
            return dir * (1.0 / i) * (car->speed - game->speed) / game->maxSpeed;
        }

        // Check each car in the segment for possible collision avoidance
        for (int j = 0; j < segment->carCount; j++) {
            Car* otherCar = segment->cars[j];
            double otherCarW = otherCar->sprite->w * SPRITE_SCALE;

            // Avoid a slower car ahead in the segment if a collision is likely
            if ((car->speed > otherCar->speed) && overlap(car->offset, carW, otherCar->offset, otherCarW, 1.2)) {
                double dir;
                // Determine direction to steer based on the other car's position
                if (otherCar->offset > 0.5)
                    dir = -1.0;  // Other car is on the right; steer left
                else if (otherCar->offset < -0.5)
                    dir = 1.0;   // Other car is on the left; steer right
                else
                    dir = (car->offset > otherCar->offset) ? 1.0 : -1.0; // Choose direction based on relative position

                // Return calculated steering adjustment
                return dir * (1.0 / i) * (car->speed - otherCar->speed) / game->maxSpeed;
            }
        }
    }

    // If no obstacles are found but the car is off the road, steer it back on
    if (car->offset < -0.9)
        return 0.1;  // Adjust right if too far left
    else if (car->offset > 0.9)
        return -0.1; // Adjust left if too far right
    else
        return 0.0;    // No steering adjustment needed
}

double calculateHorizonY(Game* game) {
    double offset = 0; // negative values move this upward
    if (game->highestRoadY > 0) {
        return game->highestRoadY + offset;
    } else {
        return (double)game->height / 2.0;
    }
}

// Comparison function for sorting leaderboard entries
int compareLeaderboardEntries(const void* a, const void* b) {
    const LeaderboardEntry* entryA = (const LeaderboardEntry*)a;
    const LeaderboardEntry* entryB = (const LeaderboardEntry*)b;
    
    if (entryA->totalRaceTime < entryB->totalRaceTime) return -1;
    if (entryA->totalRaceTime > entryB->totalRaceTime) return 1;
    return 0;
}

// Generate the leaderboard when the race ends
void generateLeaderboard(Game* game) {
    // Free any previous leaderboard entries
    if (game->leaderboard.entries) {
        free(game->leaderboard.entries);
        game->leaderboard.entries = NULL;
    }

    int numRacers = 1 + game->aiController.driverCount;
    game->leaderboard.entries = malloc(numRacers * sizeof(LeaderboardEntry));
    if (!game->leaderboard.entries) {
        SDL_Log("Failed to allocate memory for leaderboard entries.");
        return;
    }

    // Determine if the player finished first.
    // (i.e. if any AI driver finished before the player, then player did not finish first)
    bool playerFinishedFirst = true;
    for (int i = 0; i < game->aiController.driverCount; i++) {
        AIDriver* ai = &game->aiController.drivers[i];
        if (ai->finished && ai->totalRaceTime < game->totalRaceTime) {
            playerFinishedFirst = false;
            break;
        }
    }

    // If the player finished first, calculate estimated finish times for AI drivers.
    // Here we compute the remaining distance for the AI (track length minus its current position),
    // then divide by the current speed (with a safety minimum) to get an estimate of how much longer
    // it would have taken. A fudge factor is applied to account for deceleration or upcoming turns.
    if (playerFinishedFirst) {
        for (int i = 0; i < game->aiController.driverCount; i++) {
            AIDriver* ai = &game->aiController.drivers[i];
            double remainingDistance = game->road.trackLength - ai->z;
            double effectiveSpeed = (ai->speed > 0.1) ? ai->speed : 0.1;  // prevent division by zero
            double estimatedTimeRemaining = remainingDistance / effectiveSpeed;
            double fudgeFactor = 0.5; // value to match in-game deceleration or cornering effects
            estimatedTimeRemaining *= fudgeFactor;
            ai->totalRaceTime = ai->totalRaceTime + estimatedTimeRemaining;
        }
    }

    // Create player leaderboard entry
    game->leaderboard.entries[0].isPlayer = true;
    game->leaderboard.entries[0].lapsCompleted = game->playerLapsCompleted;
    game->leaderboard.entries[0].totalDistance = game->playerLapsCompleted * game->road.trackLength + game->position;
    game->leaderboard.entries[0].totalRaceTime = game->totalRaceTime;
    strcpy(game->leaderboard.entries[0].name, "Player");

    // Create AI leaderboard entries
    for (int i = 0; i < game->aiController.driverCount; i++) {
        AIDriver* ai = &game->aiController.drivers[i];
        game->leaderboard.entries[i+1].isPlayer = false;
        game->leaderboard.entries[i+1].lapsCompleted = ai->lapsCompleted;
        game->leaderboard.entries[i+1].totalDistance = ai->lapsCompleted * game->road.trackLength + ai->z;
        game->leaderboard.entries[i+1].totalRaceTime = ai->totalRaceTime;
        snprintf(game->leaderboard.entries[i+1].name, 32, "AI %d", i+1);
    }

    // Sort leaderboard entries (lowest finish time first)
    qsort(game->leaderboard.entries, numRacers, sizeof(LeaderboardEntry), compareLeaderboardEntries);
    game->leaderboard.entryCount = numRacers;

    // Create leaderboard texture
    if (game->leaderboard.texture) {
        SDL_DestroyTexture(game->leaderboard.texture);
        game->leaderboard.texture = NULL;
    }

    const int padding = 20;
    const int entryHeight = 30;
    const int entrySpacing = 5;
    int width = 400;
    int height = padding * 2 + (entryHeight + entrySpacing) * numRacers;

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        SDL_Log("Failed to create leaderboard surface: %s", SDL_GetError());
        return;
    }

    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 50));

    SDL_Color textColor = {255, 255, 255, 255};
    int y = padding;

    for (int i = 0; i < numRacers; i++) {
        LeaderboardEntry* entry = &game->leaderboard.entries[i];
        
        char posText[16], timeText[32];
        snprintf(posText, sizeof(posText), "%d", i+1);
        snprintf(timeText, sizeof(timeText), "%.2fs", entry->totalRaceTime);

        SDL_Surface* posSurf = TTF_RenderText_Blended(game->hudFont, posText, textColor);
        if (!posSurf) {
            SDL_Log("Failed to render position text: %s", TTF_GetError());
            continue;
        }
        SDL_Surface* nameSurf = TTF_RenderText_Blended(game->hudFont, entry->name, textColor);
        if (!nameSurf) {
            SDL_Log("Failed to render name text: %s", TTF_GetError());
            SDL_FreeSurface(posSurf);
            continue;
        }
        SDL_Surface* timeSurf = TTF_RenderText_Blended(game->hudFont, timeText, textColor);
        if (!timeSurf) {
            SDL_Log("Failed to render time text: %s", TTF_GetError());
            SDL_FreeSurface(posSurf);
            SDL_FreeSurface(nameSurf);
            continue;
        }

        SDL_Rect posRect = {padding, y, posSurf->w, posSurf->h};
        SDL_BlitSurface(posSurf, NULL, surface, &posRect);
        SDL_FreeSurface(posSurf);

        SDL_Rect nameRect = {padding + 50, y, nameSurf->w, nameSurf->h};
        SDL_BlitSurface(nameSurf, NULL, surface, &nameRect);
        SDL_FreeSurface(nameSurf);

        SDL_Rect timeRect = {width - padding - timeSurf->w, y, timeSurf->w, timeSurf->h};
        SDL_BlitSurface(timeSurf, NULL, surface, &timeRect);
        SDL_FreeSurface(timeSurf);

        y += entryHeight + entrySpacing;
    }

    game->leaderboard.texture = SDL_CreateTextureFromSurface(game->renderer, surface);
    if (!game->leaderboard.texture) {
        SDL_Log("Failed to create leaderboard texture: %s", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    SDL_SetTextureBlendMode(game->leaderboard.texture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surface);

    game->leaderboard.rect.x = (game->width - width) / 2;
    game->leaderboard.rect.y = (game->height - height) / 2;
    game->leaderboard.rect.w = width;
    game->leaderboard.rect.h = height;
    game->leaderboard.isActive = true;
}