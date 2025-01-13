// game.c
#include "game.h"
#include "background_game.h"
#include "background.h"
#include "util.h"
#include "render.h"
#include "constants.h"
#include "level_loader.h"

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

// Initialize the game
void initGame(Game* game, SDL_Renderer* renderer) {
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

    // Initialize input key states (0 means not pressed)
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
    if (!loadLevel(game, "/home/horatiobelter/Desktop/Programming/Pseudo 3D Racer/levels/desert_level.json")) {
        SDL_Log("Failed to load level.\n");
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

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
    // Handle main game logic
    double playerW = SPRITE_PLAYER_STRAIGHT.w * SPRITE_SCALE;

    // Percentage of the maximum speed that the player is currently moving
    double speedPercent = (game->maxSpeed > 0.0) ? (game->speed / game->maxSpeed) : 0.0; // Avoid division by zero

    // Horizontal movement increment based on time step and speed
    double dx = dt * 2.0 * speedPercent;

    // Store the starting position to calculate distance moved
    double startPosition = game->position;

    // Find the current player segment
    Segment* playerSegment = findSegment(game, game->position + game->playerZ);
    if (playerSegment == NULL) {
        SDL_Log("Error: Player segment not found in updateGame.\n");
        return;
    }

    // Update cars on the track
    updateCars(game, dt, playerSegment, playerW);

    // Move the player's position forward based on current speed
    game->position = increase(game->position, dt * game->speed, game->road.trackLength);

    // Handle player input for left/right movement
    if (game->keyLeft)
        game->playerX -= dx;
    if (game->keyRight)
        game->playerX += dx;

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

    // Limit the player’s horizontal position and speed
    game->playerX = limit(game->playerX, -3.0, 3.0);
    game->speed = limit(game->speed, 0.0, game->maxSpeed);

    // Update lap timing variables
    if (game->position > game->playerZ) {
        if (game->currentLapTime > 0.0 && (startPosition < game->playerZ)) {
            game->lastLapTime = game->currentLapTime;
            game->currentLapTime = 0.0;
            SDL_Log("Lap completed. Last lap time: %.2f seconds\n", game->lastLapTime);
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

    // Update the background sliding window
    updateBackgroundSlidingWindow(game, game->slidingWindow, dt, game->speed, playerSegment->curve);

    if (game->transitionElapsed && !game->slidingWindow->transitioning) {
        // Reset flag to prevent it from being updated every frame
        game->transitionElapsed = 0;
    }

    // Update parallax for the current background cycle state
    double elevation = calculateElevation(playerSegment, game->position);
    updateParallax(game->currentCycleState, game->speed, playerSegment->curve, elevation, game->position, game->position - (game->speed * dt), dt, game->horizonY);

    // Handle background transitions
    if (game->slidingWindow->transitioning && game->targetCycleState) {
        updateLayerTransitions(game, game->currentCycleState, game->targetCycleState, dt, game->speed, playerSegment->curve);
        updateTransitionColors(game->slidingWindow, dt);
    }
    // Automatically advance to the next background state if threshold is reached
    if (!game->slidingWindow->transitioning && game->position > game->lodCycleThreshold) {
        advanceToNextState(game, game->slidingWindow);
        game->currentCycleState = &game->slidingWindow->cycleStates[game->slidingWindow->currentCycleStateIndex];
        game->lodCycleThreshold += (game->road.trackLength / 3.0);
        if (game->lodCycleThreshold > game->road.trackLength) {
            game->lodCycleThreshold -= game->road.trackLength;
        }
    }

    if (game->slidingWindow->transitioning && game->targetCycleState) {
        double elevation2 = calculateElevation(playerSegment, game->position);
        updateParallax(game->targetCycleState, 
                   game->speed, 
                   playerSegment->curve,
                   elevation2,
                   game->position,
                   game->position - (game->speed * dt),
                   dt,
                   game->horizonY);
    }

    if (!game->slidingWindow->transitioning) {
        game->currentCycleState = &game->slidingWindow->cycleStates[game->slidingWindow->currentCycleStateIndex];
    }    
    // Update HUD elements (TODO)
}

// Render the game
void renderGame(Game* game) {
    // Call the render function
    render(game);

    // Render HUD
    // TO BE ADDED
}

// Clean up game resources
void cleanupGame(Game* game) {
    // Free resources
    if (game->background)
        SDL_DestroyTexture(game->background);
    if (game->spritesheet)
        SDL_DestroyTexture(game->spritesheet);
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
    double horizonZ = game->drawDistance * SEGMENT_LENGTH; 
    horizonZ += 200;
    double targetPos = game->position + horizonZ;

    if (targetPos >= game->road.trackLength) {
        targetPos = fmod(targetPos, game->road.trackLength);
    }

    Segment* farSegment = findSegment(game, targetPos);
    if (!farSegment) {
        return (double)game->height / 2.0; 
    }

    double segmentPercent = fmod(targetPos, SEGMENT_LENGTH) / SEGMENT_LENGTH;
    double baselineY = interpolate(farSegment->p1.world.y, farSegment->p2.world.y, segmentPercent);

    Point horizonPoint;
    horizonPoint.world.x = 0.0;          
    horizonPoint.world.y = baselineY;  
    horizonPoint.world.z = farSegment->p1.world.z; 

    double roadWidth = ROAD_WIDTH;
    double playerY = calculateElevation(findSegment(game, game->position), game->position);
    double cameraX = (game->playerX * roadWidth);
    double cameraY = playerY + CAMERA_HEIGHT;
    double cameraZ = game->position;  
    if (cameraZ >= game->road.trackLength) {
        cameraZ = fmod(cameraZ, game->road.trackLength);
    }
    
    project(&horizonPoint, cameraX, cameraY, cameraZ, game->cameraDepth, 
            game->width, game->height, roadWidth);

    return horizonPoint.screen.y;
}