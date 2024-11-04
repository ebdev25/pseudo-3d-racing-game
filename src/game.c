// game.c
#include "game.h"
#include "util.h"
#include "render.h"
#include "constants.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Forward declarations
void updateCars(Game* game, double dt, Segment* playerSegment, double playerW);
double updateCarOffset(Game* game, Car* car, Segment* carSegment, Segment* playerSegment, double playerW);
int overlap(double x1, double w1, double x2, double w2, double percent);

void initGame(Game* game, SDL_Renderer* renderer) {
    // Initialize SDL_ttf for text rendering in the HUD
    TTF_Init();

    // Set up renderer and screen dimensions
    game->renderer = renderer;
    game->width = WIDTH;               // Screen width
    game->height = HEIGHT;             // Screen height
    game->resolution = (double)game->height / 480.0;  // Scaling factor based on screen height

    // Initialize player and camera state
    game->position = 0;                // Current position along the track
    game->speed = 0;                   // Initial speed of the player
    game->playerX = 0;                 // Horizontal offset of the player from the road center
    game->playerZ = CAMERA_HEIGHT * (1 / tan((FIELD_OF_VIEW / 2.0) * M_PI / 180.0));  // Distance from camera to player
    game->cameraDepth = 1 / tan((FIELD_OF_VIEW / 2.0) * M_PI / 180.0);  // Depth of camera view

    // Initialize input key states (0 means not pressed)
    game->keyLeft = 0;
    game->keyRight = 0;
    game->keyAccelerate = 0;
    game->keyBrake = 0;

    // Load background texture
    game->background = IMG_LoadTexture(renderer, "../resources/background.png");
    if (!game->background) {
        SDL_Log("Failed to load background texture: %s", SDL_GetError());
        exit(1);
    }

    // Load spritesheet texture for in-game objects and cars
    game->spritesheet = IMG_LoadTexture(renderer, "../resources/sprites.png");
    if (!game->spritesheet) {
        SDL_Log("Failed to load spritesheet texture: %s", SDL_GetError());
        exit(1);
    }

    // Load background music
    game->music = Mix_LoadMUS("../resources/music/racer.mp3");
    if (!game->music) {
        SDL_Log("Failed to load background music: %s", Mix_GetError());
        exit(1);
    }

    // Start playing background music in a loop
    if (Mix_PlayMusic(game->music, -1) == -1) {
        SDL_Log("Failed to play music: %s", Mix_GetError());
        exit(1);
    }

    // Initialize road and track parameters
    game->road.segments = NULL;        // Array of track segments
    game->road.segmentCount = 0;       // Number of segments on the track
    game->road.trackLength = 0;        // Total length of the track
    game->road.cars = NULL;            // Array of cars on the track
    game->road.carCount = 0;           // Number of cars

    // Set various track properties
    game->rumbleLength = RUMBLE_LENGTH;  // Length of alternating rumble strip colors
    game->totalCars = 200;               // Total number of cars on the track
    game->maxSpeed = MAX_SPEED;          // Maximum player speed
    game->accel = game->maxSpeed / 5;    // Acceleration rate
    game->breaking = -game->maxSpeed;    // Deceleration (braking) rate
    game->decel = -game->maxSpeed / 5;   // Natural deceleration when not accelerating
    game->offRoadDecel = -game->maxSpeed / 2;  // Deceleration when off-road
    game->offRoadLimit = game->maxSpeed / 4;   // Speed limit when off-road
    game->centrifugal = 0.3;              // Centrifugal force for turning

    // Initialize offsets and speeds for background layers
    game->skyOffset = 0;                 // Initial offset for the sky background
    game->hillOffset = 0;                // Initial offset for the hill background
    game->treeOffset = 0;                // Initial offset for the tree background
    game->skySpeed = 0.001;              // Speed multiplier for sky background scroll
    game->hillSpeed = 0.002;             // Speed multiplier for hill background scroll
    game->treeSpeed = 0.003;             // Speed multiplier for tree background scroll

    // Camera and drawing settings
    game->lanes = LANE_COUNT;            // Number of lanes on the road
    game->drawDistance = DRAW_DISTANCE;  // Number of segments to draw ahead of the player
    game->fogDensity = FOG_DENSITY;      // Density of fog effect for distant segments

    // Initialize lap timing variables
    game->currentLapTime = 0;            // Time for the current lap
    game->lastLapTime = 0;               // Time for the previous lap

    // Initialize HUD (if implemented later)
    // TO BE ADDED

    // Initialize bg-segments
    initBackgroundSegments(game);

    // Seed the random number generator for randomized elements
    srand((unsigned int)time(NULL));

    // Build the initial road layout
    resetRoad(game);
}

void handleEvent(Game* game, SDL_Event* event) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_LEFT:
            case SDLK_a:
                game->keyLeft = 1;
                break;
            case SDLK_RIGHT:
            case SDLK_d:
                game->keyRight = 1;
                break;
            case SDLK_UP:
            case SDLK_w:
                game->keyAccelerate = 1;
                break;
            case SDLK_DOWN:
            case SDLK_s:
                game->keyBrake = 1;
                break;
        }
    } else if (event->type == SDL_KEYUP) {
        switch (event->key.keysym.sym) {
            case SDLK_LEFT:
            case SDLK_a:
                game->keyLeft = 0;
                break;
            case SDLK_RIGHT:
            case SDLK_d:
                game->keyRight = 0;
                break;
            case SDLK_UP:
            case SDLK_w:
                game->keyAccelerate = 0;
                break;
            case SDLK_DOWN:
            case SDLK_s:
                game->keyBrake = 0;
                break;
        }
    }
}

void updateGame(Game* game, double dt) {
    // Calculate player width for collision detection
    double playerW = SPRITE_PLAYER_STRAIGHT.w * SPRITE_SCALE;

    // Percentage of the maximum speed that the player is currently moving
    double speedPercent = game->speed / game->maxSpeed;

    // Horizontal movement increment based on time step and speed
    double dx = dt * 2 * speedPercent;

    // Store the starting position to calculate distance moved
    double startPosition = game->position;

    // Find the segment where the player currently is
    Segment* playerSegment = findSegment(game, game->position + game->playerZ);

    // Update the AI cars on the track
    updateCars(game, dt, playerSegment, playerW);

    // Move the player's position forward based on current speed
    game->position = increase(game->position, dt * game->speed, game->road.trackLength);

    // Handle player input for left/right movement
    if (game->keyLeft)
        game->playerX -= dx;
    else if (game->keyRight)
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
    if ((game->playerX < -1) || (game->playerX > 1)) {
        // Slow down player if off-road
        if (game->speed > game->offRoadLimit)
            game->speed = accelerate(game->speed, game->offRoadDecel, dt);

        // Check for collision with roadside sprites
        for (int n = 0; n < playerSegment->spriteCount; n++) {
            SpriteInstance* sprite = &playerSegment->sprites[n];
            double spriteW = sprite->source->w * SPRITE_SCALE;
            // If collision detected, slow down player and reset position slightly
            if (overlap(game->playerX, playerW, sprite->offset + spriteW / 2 * (sprite->offset > 0 ? 1 : -1), spriteW, 1.0)) {
                game->speed = game->maxSpeed / 5;
                game->position = increase(playerSegment->p1.world.z, -game->playerZ, game->road.trackLength);
                break;
            }
        }
    }

    // Check for collisions with other cars
    for (int n = 0; n < playerSegment->carCount; n++) {
        Car* car = playerSegment->cars[n];
        double carW = car->sprite->w * SPRITE_SCALE;
        if (game->speed > car->speed) {
            // If a collision is detected, reduce player speed and reset position
            if (overlap(game->playerX, playerW, car->offset, carW, 0.8)) {
                game->speed = car->speed * (car->speed / game->speed);
                game->position = increase(car->z, -game->playerZ, game->road.trackLength);
                break;
            }
        }
    }

    // Limit the player’s horizontal position and speed
    game->playerX = limit(game->playerX, -3, 3);
    game->speed = limit(game->speed, 0, game->maxSpeed);

    // Find the player's updated segment after position change
    Segment* playerSeg = findSegment(game, game->position);

    // Update the background layers' scrolling offsets based on road curvature
    game->skyOffset = increase(game->skyOffset, game->skySpeed * playerSeg->curve * (game->position - startPosition) / SEGMENT_LENGTH, 1);
    game->hillOffset = increase(game->hillOffset, game->hillSpeed * playerSeg->curve * (game->position - startPosition) / SEGMENT_LENGTH, 1);
    game->treeOffset = increase(game->treeOffset, game->treeSpeed * playerSeg->curve * (game->position - startPosition) / SEGMENT_LENGTH, 1);

    // Update the lap timer based on player position
    if (game->position > game->playerZ) {
        // If crossing the start line, reset lap time and save last lap time
        if (game->currentLapTime > 0 && startPosition < game->playerZ) {
            game->lastLapTime = game->currentLapTime;
            game->currentLapTime = 0;
            // TODO: HUD updates for lap times can be added here
        } else {
            // Increment lap timer if not crossing the start line
            game->currentLapTime += dt;
        }
    }

    // Update HUD elements
    // TODO: Add HUD update code here
}

void renderGame(Game* game) {
    // Call the render function
    render(game);

    // Render HUD
    // TO BE ADDED
}

void cleanupGame(Game* game) {
    // Free resources
    if (game->background)
        SDL_DestroyTexture(game->background);
    if (game->spritesheet)
        SDL_DestroyTexture(game->spritesheet);

    // Free road segments
    for (int i = 0; i < game->road.segmentCount; i++) {
        Segment* segment = &game->road.segments[i];
        if (segment->sprites)
            free(segment->sprites);
        if (segment->cars)
            free(segment->cars);
    }
    free(game->road.segments);

    // Free cars
    free(game->road.cars);

    // Free music
    if (game->music)
        Mix_FreeMusic(game->music);

    // Quit SDL_ttf
    TTF_Quit();
}

void updateCars(Game* game, double dt, Segment* playerSegment, double playerW) {
    // Loop through each car on the road
    for (int n = 0; n < game->road.carCount; n++) {
        Car* car = &game->road.cars[n];

        // Find the segment the car is currently in
        Segment* oldSegment = findSegment(game, car->z);

        // Adjust the car's lateral position based on player position and obstacles
        double offset = updateCarOffset(game, car, oldSegment, playerSegment, playerW);
        car->offset += offset;

        // Move the car forward based on its speed and elapsed time
        car->z = increase(car->z, dt * car->speed, game->road.trackLength);

        // Calculate car's position as a percentage of the current segment
        car->percent = percentRemaining(car->z, SEGMENT_LENGTH);

        // Find the segment the car has moved into
        Segment* newSegment = findSegment(game, car->z);

        // Check if the car has moved to a different segment
        if (oldSegment != newSegment) {
            // Remove car from the old segment's car list
            for (int i = 0; i < oldSegment->carCount; i++) {
                if (oldSegment->cars[i] == car) {
                    // Shift remaining cars to fill the gap
                    for (int j = i; j < oldSegment->carCount - 1; j++) {
                        oldSegment->cars[j] = oldSegment->cars[j + 1];
                    }
                    oldSegment->carCount--;
                    break;
                }
            }

            // Add car to the new segment's car list
            newSegment->cars = realloc(newSegment->cars, sizeof(Car*) * (newSegment->carCount + 1));
            newSegment->cars[newSegment->carCount] = car;
            newSegment->carCount++;
        }
    }
}

double updateCarOffset(Game* game, Car* car, Segment* carSegment, Segment* playerSegment, double playerW) {
    int lookahead = 20; // Number of segments to look ahead to avoid collisions
    double carW = car->sprite->w * SPRITE_SCALE; // Width of the car in game units

    // If the car is too far from the player, skip adjusting its offset
    if ((carSegment->index - playerSegment->index) > game->drawDistance)
        return 0;

    // Check each segment in the lookahead distance for potential obstacles
    for (int i = 1; i < lookahead; i++) {
        Segment* segment = &game->road.segments[(carSegment->index + i) % game->road.segmentCount];

        // Avoid the player if they are in the segment and in front of the car
        if ((segment == playerSegment) && (car->speed > game->speed) &&
            overlap(game->playerX, playerW, car->offset, carW, 1.2)) {

            double dir;
            // Determine which direction to steer based on player's position
            if (game->playerX > 0.5)
                dir = -1;  // Player is on the right; steer left
            else if (game->playerX < -0.5)
                dir = 1;   // Player is on the left; steer right
            else
                dir = (car->offset > game->playerX) ? 1 : -1; // Choose direction based on relative position

            // Return calculated steering adjustment
            return dir * 1.0 / i * (car->speed - game->speed) / game->maxSpeed;
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
                    dir = -1;  // Other car is on the right; steer left
                else if (otherCar->offset < -0.5)
                    dir = 1;   // Other car is on the left; steer right
                else
                    dir = (car->offset > otherCar->offset) ? 1 : -1; // Choose direction based on relative position

                // Return calculated steering adjustment
                return dir * 1.0 / i * (car->speed - otherCar->speed) / game->maxSpeed;
            }
        }
    }

    // If no obstacles are found but the car is off the road, steer it back on
    if (car->offset < -0.9)
        return 0.1;  // Adjust right if too far left
    else if (car->offset > 0.9)
        return -0.1; // Adjust left if too far right
    else
        return 0;    // No steering adjustment needed
}

// Checks if two objects overlap based on their positions and widths.
int overlap(double x1, double w1, double x2, double w2, double percent) {
    // Calculate half of the combined widths, scaled by the overlap threshold percentage.
    double half = (w1 + w2) * (percent / 2.0);

    // Check if the distance between x1 and x2 is less than the calculated half width.
    // If true, it means the objects are within the overlap threshold.
    return fabs(x1 - x2) < half;
}

void initBackgroundSegments(Game* game) {
    game->backgroundSegments[0] = (BackgroundSegment) {
        .texture = IMG_LoadTexture(game->renderer, "new_sky.png"),
        .parallaxSpeed = 0.1,
        .blendStartDistance = 0,
        .blendEndDistance = 500,
    };
    game->backgroundSegments[1] = (BackgroundSegment) {
        .texture = IMG_LoadTexture(game->renderer, "mountains.png"),
        .parallaxSpeed = 0.5,
        .blendStartDistance = 200,
        .blendEndDistance = 800,
    };
    game->backgroundSegments[2] = (BackgroundSegment) {
        .texture = IMG_LoadTexture(game->renderer, "trees.png"),
        .parallaxSpeed = 0.1,
        .blendStartDistance = 500,
        .blendEndDistance = 1000,
    };
}