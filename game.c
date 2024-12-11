// game.c
#include "game.h"
#include "util.h"
#include "render.h"
#include "constants.h"

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
    game->width = WIDTH;               // Screen width
    game->height = HEIGHT;             // Screen height
    game->resolution = (double)game->height / 480.0;  // Scaling factor based on screen height

    // Initialize player and camera state
    game->position = 0.0;                // Current position along the track
    game->speed = 0.0;                   // Initial speed of the player
    game->playerX = 0.0;                 // Horizontal offset of the player from the road center
    game->playerZ = CAMERA_HEIGHT * (1.0 / tan((FIELD_OF_VIEW / 2.0) * M_PI / 180.0));  // Distance from camera to player
    game->cameraDepth = 1.0 / tan((FIELD_OF_VIEW / 2.0) * M_PI / 180.0);  // Depth of camera view

    // Initialize input key states (0 means not pressed)
    game->keyLeft = 0;
    game->keyRight = 0;
    game->keyAccelerate = 0;
    game->keyBrake = 0;

    // Load background texture
    game->background = IMG_LoadTexture(renderer, "../resources/background_v11.png");
    if (!game->background) {
        SDL_Log("Failed to load background texture: %s", SDL_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

    // Query background texture width
    int bgWidth, bgHeight;
    SDL_QueryTexture(game->background, NULL, NULL, &bgWidth, &bgHeight);
    game->cycleStateWidth = (double)bgWidth;
    SDL_Log("Background texture width set to cycleStateWidth: %.2f pixels", game->cycleStateWidth);


    // Load spritesheet texture for in-game objects and cars
    game->spritesheet = IMG_LoadTexture(renderer, "../resources/sprites_v2.png");
    if (!game->spritesheet) {
        SDL_Log("Failed to load spritesheet texture: %s", SDL_GetError());
        SDL_DestroyTexture(game->background);
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

    // Load background music
    game->music = Mix_LoadMUS("../resources/music/racer.mp3");
    if (!game->music) {
        SDL_Log("Failed to load background music: %s", Mix_GetError());
        SDL_DestroyTexture(game->spritesheet);
        SDL_DestroyTexture(game->background);
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

    // Start playing background music in a loop
    if (Mix_PlayMusic(game->music, -1) == -1) {
        SDL_Log("Failed to play music: %s", Mix_GetError());
        Mix_FreeMusic(game->music);
        SDL_DestroyTexture(game->spritesheet);
        SDL_DestroyTexture(game->background);
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

    // Initialize road and track parameters
    game->road.segments = NULL;        // Array of track segments
    game->road.segmentCount = 0;       // Number of segments on the track
    game->road.trackLength = 0.0;      // Total length of the track
    game->road.cars = NULL;            // Array of cars on the track
    game->road.carCount = 0;           // Number of cars

    // Set various track properties
    game->rumbleLength = RUMBLE_LENGTH;      // Length of alternating rumble strip colors
    game->totalCars = 200;                   // Total number of cars on the track
    game->maxSpeed = MAX_SPEED;              // Maximum player speed
    game->accel = game->maxSpeed / 5.0;      // Acceleration rate
    game->breaking = -game->maxSpeed;        // Deceleration (braking) rate
    game->decel = -game->maxSpeed / 5.0;     // Natural deceleration when not accelerating
    game->offRoadDecel = -game->maxSpeed / 2.0;  // Deceleration when off-road
    game->offRoadLimit = game->maxSpeed / 4.0;   // Speed limit when off-road
    game->centrifugal = 0.3;                  // Centrifugal force for turning

    // Initialize offsets and speeds for background layers
    game->skyOffset = 0.0;
    game->hillOffset = 0.0;
    game->treeOffset = 0.0;
    game->mountainOffset = 0.0;

    game->skySpeed = 0.0001;
    game->hillSpeed = 0.0015;
    game->treeSpeed = 0.003;
    game->mountainSpeed = 0.001; // Corrected from mountainOffset to mountainSpeed

    // Initialize transition variables
    game->transitionPhase = TRANSITION_NONE;
    game->isTransitioning = false;
    game->transitionProgress = 0.0;
    game->transitionSpeed = 0.5;  // Desired speed

    // Initialize parallax scrolling flag
    game->parallaxEnabled = true;

    // Initialize parallax lock
    game->parallaxLock = true;

    // Initialize fixed parallax offsets
    game->fixedHillOffset = 0.0;
    game->fixedTreeOffset = 0.0;
    game->fixedMountainOffset = 0.0;

    // Background fog settings
    game->fogEnabled = false;             // Initially Off
    game->fogIntensity = 0.0;             // No fog initially
    game->fogTransitionSpeed = 1.0;
    game->fogColor = (SDL_Color){135, 206, 235, 255}; // Sky blue

    // Camera and drawing settings
    game->lanes = LANE_COUNT;              // Number of lanes on the road
    game->drawDistance = DRAW_DISTANCE;    // Number of segments to draw ahead of the player
    game->fogDensity = FOG_DENSITY;        // Density of fog effect for distant segments

    // Initialize lap timing variables
    game->currentLapTime = 0.0;            // Time for the current lap
    game->lastLapTime = 0.0;               // Time for the previous lap

    // Initialize HUD (if implemented later)
    // TO BE ADDED

    // Seed the random number generator for randomized elements
    srand((unsigned int)time(NULL));

    // Build the initial road layout
    resetRoad(game);

    game->slidingWindow = createSlidingWindow();

    // Set background LOD cycling threshold after road is initialized
    game->lodCycleThreshold = (game->road.trackLength > 0.0) ? (game->road.trackLength / 3.0) : 1000.0;  // Divide track into three sections or set default

    // Initialize blended texture arrays to NULL
    for (int state = 0; state < BACKGROUND_CYCLE_STATES_COUNT; ++state) {
        game->blendedHighLODTextures[state] = NULL;
        game->blendedMidLODTextures[state] = NULL;
    }

    // Create blended textures for each cycle state
    SDL_Surface* backgroundSurface = IMG_Load("../resources/newer_textures_full.png");
    if (!backgroundSurface) {
        SDL_Log("Failed to load background surface for blending: %s", IMG_GetError());
        cleanupGame(game);
        exit(1);
    }

    // Define the blend color (e.g., sky blue)
    SDL_Color blendColor = {135, 206, 235, 255}; // RGB: 135, 206, 235

    // Iterate through each background cycle state to create blended textures
    for (int state = 0; state < BACKGROUND_CYCLE_STATES_COUNT; ++state) {
        // High LOD (L4)
        SDL_Rect l4Rect = *(BACKGROUND_CYCLE_STATES[state].L4);
        game->blendedHighLODTextures[state] = create_blended_texture(renderer, backgroundSurface, l4Rect, blendColor, HIGH_LOD_BLEND_FACTOR);
        if (!game->blendedHighLODTextures[state]) {
            SDL_Log("Failed to create blended High LOD texture for state %d.\n", state);
            // Handle error as needed (e.g., continue, exit, etc.)
        }

        // Mid LOD (L3)
        SDL_Rect l3Rect = *(BACKGROUND_CYCLE_STATES[state].L3);
        game->blendedMidLODTextures[state] = create_blended_texture(renderer, backgroundSurface, l3Rect, blendColor, MID_LOD_BLEND_FACTOR);
        if (!game->blendedMidLODTextures[state]) {
            SDL_Log("Failed to create blended Mid LOD texture for state %d.\n", state);
            // Handle error as needed
        }

        // No blending for Low LOD (L2)
    }

    // Free the loaded background surface as it's no longer needed
    SDL_FreeSurface(backgroundSurface);

    // Initialize saturation flag
    game->saturationOn = false;
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
    // Handle transition phases using SlidingWindow
    if (game->slidingWindow->transitioning) {
        // Update transition progress with easing
        game->slidingWindow->progress += SLIDE_SPEED * dt;
        game->slidingWindow->progress = limit(game->slidingWindow->progress, 0.0f, 1.0f); // Clamp between 0 and 1

        // Apply easing function for smooth transition
        double easedProgress = easeInOut(0.0, 1.0, (double)game->slidingWindow->progress);

        // Log the eased progress for debugging
        SDL_Log("UpdateGame - Transition Progress: %.2f, Eased Progress: %.2f", game->slidingWindow->progress, easedProgress);

        if (game->slidingWindow->progress >= 1.0f) {
            // Transition complete
            finalizeTransition(game->slidingWindow);
        }

        // Scroll out the current cycle state based on eased progress
        game->hillOffset = interpolate(game->hillOffset, game->hillOffset - game->cycleStateWidth, easedProgress);
        game->treeOffset = interpolate(game->treeOffset, game->treeOffset - game->cycleStateWidth, easedProgress);
        game->mountainOffset = interpolate(game->mountainOffset, game->mountainOffset - game->cycleStateWidth, easedProgress);

        // Note: Next background's offsets are handled in the render function using easedProgress
    }

    // Handle main game logic
    // Calculate player width for collision detection
    double playerW = SPRITE_PLAYER_STRAIGHT.w * SPRITE_SCALE;

    // Percentage of the maximum speed that the player is currently moving
    double speedPercent = (game->maxSpeed > 0.0) ? (game->speed / game->maxSpeed) : 0.0; // Avoid division by zero

    // Horizontal movement increment based on time step and speed
    double dx = dt * 2.0 * speedPercent;

    // Store the starting position to calculate distance moved
    double startPosition = game->position;

    // Determine the current phase based on player's position
    int currentPhase = 0;
    if (game->lodCycleThreshold > 0.0) {
        currentPhase = (int)(game->position / game->lodCycleThreshold) % 3;
        if (currentPhase < 0) {
            currentPhase += 3; // Ensure phase is non-negative
        }
    } else {
        SDL_Log("Warning: lodCycleThreshold is zero or negative.\n");
    }

    // Check if we need to start a transition
    if (currentPhase != game->slidingWindow->currentStateIndex && !game->slidingWindow->transitioning) {
        // Start the transition by initiating the active transition phase
        bool forward = (currentPhase > game->slidingWindow->currentStateIndex) || 
                       (game->slidingWindow->currentStateIndex == 2 && currentPhase == 0);
        startTransition(game->slidingWindow, forward);
    }

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

            // Collision detection using the overlap function from util.c
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
            // If a collision is detected, reduce player speed and reset position
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

    // Update the background layers' scrolling offsets based on road curvature
    game->skyOffset = increase(game->skyOffset, game->skySpeed * playerSegment->curve * (game->position - startPosition) / SEGMENT_LENGTH, 1.0);
    game->hillOffset = increase(game->hillOffset, game->hillSpeed * playerSegment->curve * (game->position - startPosition) / SEGMENT_LENGTH, 1.0);
    game->treeOffset = increase(game->treeOffset, game->treeSpeed * playerSegment->curve * (game->position - startPosition) / SEGMENT_LENGTH, 1.0);
    game->mountainOffset = increase(game->mountainOffset, game->mountainSpeed * playerSegment->curve * (game->position - startPosition) / SEGMENT_LENGTH, 1.0);

    // Update lap timing variables
    if (game->position > game->playerZ) {
        // If crossing the start line, reset lap time and save last lap time
        if (game->currentLapTime > 0.0 && (startPosition < game->playerZ)) {
            game->lastLapTime = game->currentLapTime;
            game->currentLapTime = 0.0;
            SDL_Log("Lap completed. Last lap time: %.2f seconds\n", game->lastLapTime);
            // TODO: HUD updates for lap times can be added here
        } else {
            // Increment lap timer if not crossing the start line
            game->currentLapTime += dt;
        }
    }

    // Update HUD elements
    // TODO: Add HUD update code here
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

    // Free blended High LOD textures
    for (int state = 0; state < BACKGROUND_CYCLE_STATES_COUNT; ++state) {
        if (game->blendedHighLODTextures[state]) {
            SDL_DestroyTexture(game->blendedHighLODTextures[state]);
            game->blendedHighLODTextures[state] = NULL;
        }
    }

    // Free blended Mid LOD textures
    for (int state = 0; state < BACKGROUND_CYCLE_STATES_COUNT; ++state) {
        if (game->blendedMidLODTextures[state]) {
            SDL_DestroyTexture(game->blendedMidLODTextures[state]);
            game->blendedMidLODTextures[state] = NULL;
        }
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
                // Optionally, handle the error, e.g., remove the car or exit
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