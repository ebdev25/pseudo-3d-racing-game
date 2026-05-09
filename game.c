// game.c
#include "game.h"
#include "background_game.h"
#include "background.h"
#include "util.h"
#include "render.h"
#include "constants.h"
#include "level_loader.h"
#include "paths.h"
#include "ai.h"
#include "collision.h"
#include "leaderboard.h"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdalign.h>

#define FRAME_ARENA_BYTES (16u * 1024u * 1024u) // 16 mb of memory
#define LEVEL_ARENA_BYTES (64u * 1024u * 1024u) // 64 mb of memory
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

        // If moved beyond last frame
        if (anim->currentFrame >= anim->frameCount) {
            anim->currentFrame = anim->frameCount - 1; // clamp to final
            anim->finished = true;
            // Once green = race start:
            game->raceState = RACE_STATE_RUNNING;
            game->movementAllowed = true;

            if (game->music) { // Check if music was loaded
                Mix_PlayMusic(game->music, -1); // Start looping background music
            }

            return;
        }
    }
}

static void updateAIAndCollisions(Game* game, double dt, Segment* playerSeg) {
    double playerW = SPRITE_PLAYER_STRAIGHT.w * SPRITE_SCALE;

    updateAIController(&game->aiController, dt);
    checkAIDriverCollisions(game);
    checkPlayerAIOpponentCollisions(game);

    // AI ramming logic
    for (int i = 0; i < game->aiController.driverCount; i++) {
        AIDriver* ai = &game->aiController.drivers[i];
        Segment* aiSeg = findSegment(game, ai->z);
        if (aiSeg != playerSeg) continue;

        double aiW = (ai->sprite ? ai->sprite->w : 80) * SPRITE_SCALE;

        if (ai->isRamming && overlap(game->playerX, playerW, ai->offset, aiW, 1.0)) {
            double push = 0.1 * dt * 60;
            game->playerX += (ai->offset < game->playerX) ? push : -push;
        }
    }
}

static void updatePlayerPhysics(Game* game, double dt, Segment* seg) {
    double speedPercent = (game->maxSpeed > 0.0) ? (game->speed / game->maxSpeed) : 0.0;
    double dx = dt * 2.0 * speedPercent;

    game->position = increase(game->position, dt * game->speed, game->road.trackLength);

    if (!game->inCollisionPush) {
        if (game->keyLeft)  game->playerX -= dx;
        if (game->keyRight) game->playerX += dx;
    }

    game->playerX -= dx * speedPercent * seg->curve * game->centrifugal;

    if (game->keyAccelerate)
        game->speed = accelerate(game->speed, game->accel, dt);
    else if (game->keyBrake)
        game->speed = accelerate(game->speed, game->breaking, dt);
    else
        game->speed = accelerate(game->speed, game->decel, dt);

    game->playerX = limit(game->playerX, -3.0, 3.0);
    game->speed   = limit(game->speed, 0.0, game->maxSpeed);
}

static void handleOffRoadCollisions(Game* game, double dt, Segment* seg) {
    if (game->playerX > -1.0 && game->playerX < 1.0)
        return;

    if (game->speed <= game->offRoadLimit)
        return;

    game->speed = accelerate(game->speed, game->offRoadDecel, dt);

    for (int i = 0; i < seg->spriteCount; i++) {
        SpriteInstance* s = &seg->sprites[i];
        double spriteW = (s->source->useCustomCollisionBox ?
                          s->source->collisionWidth : s->source->w) * SPRITE_SCALE;
        double offsetX  = (s->source->useCustomCollisionBox ?
                           s->source->collisionOffsetX : 0.0) * SPRITE_SCALE;

        double center = s->offset + offsetX;

        if (overlap(game->playerX, SPRITE_PLAYER_STRAIGHT.w * SPRITE_SCALE, center, spriteW, 1.0)) {
            game->speed = game->maxSpeed / 5.0;
            game->position = increase(seg->p1.world.z, -game->playerZ, game->road.trackLength);
            game->collisionResetOccurredThisFrame = true;
            return;
        }
    }
}

static void updateLapLogic(Game* game, double startPosition) {
    if (game->position >= startPosition || game->collisionResetOccurredThisFrame)
        return;

    if (!(game->position < startPosition && startPosition > game->playerZ))
        return;

    if (game->currentLapTime <= 1.0)
        return;

    // crossed start line
    game->playerLapsCompleted++;
    game->lastLapTime = game->currentLapTime;
    game->currentLapTime = 0.0;

    if (game->playerLapsCompleted >= game->totalLaps && !game->raceFinished) {
        game->raceState = RACE_STATE_FINISHED;
        game->movementAllowed = false;
        game->raceFinished = true;

        Mix_HaltMusic();
        generateLeaderboard(game);
        game->showLeaderboard = true;

        bool won = (game->leaderboard.isActive &&
                    game->leaderboard.entryCount > 0 &&
                    game->leaderboard.entries[0].isPlayer);

        Mix_PlayChannel(-1, won ? game->victorySound : game->loseSound, 0);
    }

    game->lodCycleThreshold = game->road.trackLength / 3.0;
}

static void updateBackgroundParallax(Game* game, Segment* playerSegment, double startPosition, double dt)
{
    // Parallax updates only when not transitioning and system is valid
    if (!game->transitionHappening && game->slidingWindow && game->currentCycleState) {

        // --- First parallax update (baseline) ---
        double elevation = calculateElevation(playerSegment, game->position);
        updateParallax(
            game,
            game->slidingWindow,
            game->currentCycleState,
            game->speed,
            playerSegment->curve,
            elevation,
            game->position,
            startPosition,
            dt,
            game->horizonY,
            game->transitionHappening
        );
    }

    // Final parallax sync before transitions; uses updated segment/elevation to prevent visual pops
    if (!game->transitionHappening && game->slidingWindow && game->currentCycleState) {

        Segment* currentSegmentForParallax = findSegment(game, game->position + game->playerZ);
        if (currentSegmentForParallax) {

            double elevation = calculateElevation(currentSegmentForParallax, game->position);
            double previousPosition = startPosition;  // exactly the same as before

            updateParallax(
                game,
                game->slidingWindow,
                game->currentCycleState,
                game->speed,
                currentSegmentForParallax->curve,
                elevation,
                game->position,
                previousPosition,
                dt,
                game->horizonY,
                false     // Always false for the sync pass
            );
        }
        else {
            SDL_Log("Warning: Player segment null before pre-transition parallax update.");
        }
    }
}

void resetGameForStart(Game* game) {
    SDL_Log("Resetting game state for start/restart...");

    // Reset Player State
    game->position = 0.0;
    game->speed = 0.0;
    game->playerX = 0.0;

    // Reset Timing
    game->currentLapTime = 0.0;
    game->lastLapTime = 0.0;
    game->totalRaceTime = 0.0;
    game->playerLapsCompleted = 0;

    // Reset Race State Flags
    game->raceFinished = false;
    game->showLeaderboard = false; // Explicitly hide leaderboard
    if (game->leaderboard.texture) { // Destroy old leaderboard texture if exists
         SDL_DestroyTexture(game->leaderboard.texture);
         game->leaderboard.texture = NULL;
    }
    game->leaderboard.isActive = false; // Deactivate leaderboard logic
    game->raceState = RACE_STATE_COUNTDOWN; // Set state back to countdown
    game->movementAllowed = false; // Disallow movement initially

    // Reset Input Keys
    game->keyLeft = 0;
    game->keyRight = 0;
    game->keyAccelerate = 0;
    game->keyBrake = 0;

    // Reset AI Controller and Drivers
    if (game->aiController.drivers && game->aiController.driverCount > 0) {
        const double base_z_offset = SEGMENT_LENGTH * 15.0;
        const double z_stagger     = SEGMENT_LENGTH * 5.0;
        const double lat_positions[3] = {-0.7, 0.0, 0.7};
        const int max_row_width = 3;
        for (int i = 0; i < game->aiController.driverCount; i++) {
            AIDriver* driver = &game->aiController.drivers[i];
            double assigned_z;
            double assigned_offset;
            int row = i / max_row_width;
            int pos_in_row = i % max_row_width;

            if (row == 0) { // First row (max 3 drivers: i=0, 1, 2)
                assigned_z = base_z_offset;
                assigned_offset = lat_positions[pos_in_row];
            } else if (row == 1) { // Second row (drivers i=3, 4)
                assigned_z = base_z_offset - z_stagger; // Place behind first row
                // Re-use lateral pos (e.g., outer two)
                assigned_offset = lat_positions[pos_in_row == 0 ? 0 : 2];
            } else {
                 // Fallback for drivers beyond the first two rows
                 assigned_z = base_z_offset - (z_stagger * row);
                 assigned_offset = lat_positions[pos_in_row];
                 SDL_Log("Warning: Placing AI driver %d in fallback starting position during reset.", i);
            }

            // Assign the calculated positions (Z relative to game->position)
            driver->z = game->position + assigned_z; // Relative Z
            driver->offset = assigned_offset;
            driver->targetLaneOffset = assigned_offset;
            driver->defaultLaneOffset = assigned_offset;
            driver->speed = 0.0;
            driver->lapsCompleted = 0;
            driver->lapTime = 0.0;
            driver->bestLapTime = 1e9;
            driver->finished = false;
            driver->totalRaceTime = 0.0;
            driver->inCollision = false;
            driver->isBeingPushed = false;
            driver->isRamming = false;
            driver->ramCooldown = 0.0;
            driver->currentBehavior = AI_BEHAVIOR_NONE; // Reset behavior
            driver->behaviorElapsed = 0.0;
            driver->recovering = 0;
            driver->initialBehaviorDelayTimer = 2.0;
        }
    }

    // START: Background Reset Logic
    if (game->slidingWindow && game->slidingWindow->cycleStates && game->slidingWindow->cycleStateCount > 0) {
        SDL_Log("Resetting background state...");
        // Reset Sliding Window Indices and State
        game->slidingWindow->currentCycleStateIndex = 0;
        game->slidingWindow->targetCycleStateIndex = (game->slidingWindow->cycleStateCount > 1) ? 1 : -1; // 0 if one state
        game->slidingWindow->transitionProgress = 0.0f;
        game->slidingWindow->transitioning = false;
        game->slidingWindow->transitionElapsed = false;
        game->slidingWindow->transitionStartPosition = 0.0; // Reset transition trigger point
        game->transitionHappening = 0; // Reset game-level flag
        game->lodCycleThreshold = game->road.trackLength / 3.0; // Reset LOD trigger

        // Reset Pointers to Current/Target States
        game->currentCycleState = &game->slidingWindow->cycleStates[0];
        game->targetCycleState = (game->slidingWindow->cycleStateCount > 1) ? &game->slidingWindow->cycleStates[1] : NULL;

        // Reset Interpolated Colors
        game->slidingWindow->interpolatedSkyColor = game->slidingWindow->cycleStates[0].skyColor;
        game->slidingWindow->interpolatedRoadColor = game->slidingWindow->cycleStates[0].roadColor;

        // Reset Layer Offsets for all cycle states
        double initialHorizonY = (double)game->height / 2.0; // recalculate properly if needed
        game->horizonY = initialHorizonY; // Update game's horizon value

        for (int stateIdx = 0; stateIdx < game->slidingWindow->cycleStateCount; stateIdx++) {
             BackgroundCycleState *state = &game->slidingWindow->cycleStates[stateIdx];
             for (int layerIdx = 0; layerIdx < 3; layerIdx++) {
                 // Reset X offset
                 state->offsets[layerIdx].x = 0; // where initial X is 0

                 // Reset Y offset based on the initial horizon
                float initialY = (float)initialHorizonY - state->textureRects[layerIdx].h + state->overlapMargins[layerIdx];
                state->offsets[layerIdx].y = initialY;

                 // Reset internal transition/slide offsets if used
                 state->offsets[layerIdx].transitionX = 0;
                 state->offsets[layerIdx].slideOffsetX = 0.0f;
                 state->extraSlice[layerIdx] = 0; // Reset extra slice info
             }
         }

         // Reset Stored Offsets in Game struct (based on the now-reset state 0)
        for (int i = 0; i < 3; i++) {
            game->storedOffsets[i] = game->slidingWindow->cycleStates[0].offsets[i].x;
            game->storedOffsetsY[i] = game->slidingWindow->cycleStates[0].offsets[i].y;
         }

        SDL_Log("Background state reset.");

    } else {
         SDL_Log("Warning: Sliding window or cycle states invalid during reset.");
    }

    // Reset Traffic Light Animation
    game->trafficLightAnim.active = true; // active for countdown
    game->trafficLightAnim.finished = false;
    game->trafficLightAnim.currentFrame = 0;
    game->trafficLightAnim.currentTime = 0.0;

    game->collisionResetOccurredThisFrame = false;

    // UI set to hidden
    setUIState(&game->ui, UI_STATE_NONE);

    // Stop sounds from previous state
    Mix_HaltMusic(); // Stop everything before countdown sound

    SDL_Log("Game state reset complete.");
}

// Initialize the game (SDL_Init / TTF_Init / Mix_OpenAudio / IMG_Init: owned by main.c)
bool initGame(Game* game, SDL_Renderer* renderer, const char* levelRelPath) {
    game->frameArenaCap = FRAME_ARENA_BYTES;
    game->frameArenaStart = SDL_malloc(game->frameArenaCap);
    if (!game->frameArenaStart) {
        SDL_Log("Allocation for frame arena start failed: (%zu bytes)", game->frameArenaCap);
        return false;
    }
    arena_init(&game->frameArena, game->frameArenaStart, game->frameArenaCap);

    game->levelArenaCap = LEVEL_ARENA_BYTES;
    game->levelArenaStart = SDL_malloc(game->levelArenaCap);
    if (!game->levelArenaStart) {
        SDL_Log("Allocation for level arena start failed: (%zu bytes)", game->levelArenaCap);
        SDL_free(game->frameArenaStart);
        game->frameArenaStart = NULL;
        return false;
    }
    arena_init(&game->levelArena, game->levelArenaStart, game->levelArenaCap);

    game->raceState = RACE_STATE_COUNTDOWN;
    game->movementAllowed = false;

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

    // Set track properties
    game->rumbleLength = RUMBLE_LENGTH;  // Length of alternating rumble strip colors
    game->maxSpeed = MAX_SPEED;          // Max player speed
    game->accel = game->maxSpeed / 5.0;  // Acceleration rate
    game->breaking = -game->maxSpeed;    // Deceleration (braking) rate
    game->decel = -game->maxSpeed / 5.0; // Natural deceleration when not accelerating
    game->offRoadDecel = -game->maxSpeed / 2.0;  // Deceleration when off-road
    game->offRoadLimit = game->maxSpeed / 4.0;   // Speed limit when off-road
    game->centrifugal = 0.3;  // Centrifugal force for turning

    // Camera and drawing settings
    game->lanes = LANE_COUNT;              // Number of lanes on the road
    game->drawDistance = DRAW_DISTANCE;    // Number of segments to draw ahead of the player

    // Initialize lap timing variables
    game->currentLapTime = 0.0;  // Time for the current lap
    game->lastLapTime = 0.0;     // Time for the previous lap

    // Seed random int gen for random functionality
    srand((unsigned int)time(NULL));

    // Sliding Window init to NULL
    game->slidingWindow = NULL;

    const char* levelPath = (levelRelPath && levelRelPath[0]) ? levelRelPath : DEFAULT_LEVEL_REL_PATH;
    if (!loadLevel(game, levelPath)) {
        SDL_Log("Failed to load level: %s", levelPath);
        goto fail;
    }

    char assetPath[1024];

    if (!paths_resolve(assetPath, sizeof(assetPath), "resources/animated_sprites.png")) {
        SDL_Log("Failed to find resources/animated_sprites.png");
        goto fail;
    }
    SDL_Texture* animTex = IMG_LoadTexture(renderer, assetPath);
    if (!animTex) {
        SDL_Log("Failed to load animated sprites: %s", IMG_GetError());
        goto fail;
    }
    game->animatedSpritesheet = animTex;

    if (!paths_resolve(assetPath, sizeof(assetPath), "resources/racing_lights_sound_longer.mp3")) {
        SDL_Log("Failed to find resources/racing_lights_sound_longer.mp3");
        goto fail;
    }
    game->trafficLightSound = Mix_LoadMUS(assetPath);
    if (!game->trafficLightSound) {
        SDL_Log("Failed to load traffic light sound: %s", Mix_GetError());
        goto fail;
    }

    if (!paths_resolve(assetPath, sizeof(assetPath), "resources/PressStart2P-Regular.ttf")) {
        SDL_Log("Failed to find resources/PressStart2P-Regular.ttf");
        goto fail;
    }
    game->hudFont = TTF_OpenFont(assetPath, 24);
    if (!game->hudFont) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        goto fail;
    }

    initUI(&game->ui, game->hudFont);
    setUIState(&game->ui, UI_STATE_START_MENU);

    if (!paths_resolve(assetPath, sizeof(assetPath), "resources/uiNavSound.wav")) {
        SDL_Log("Failed to find resources/uiNavSound.wav");
        goto fail;
    }
    game->uiNavigationSound = Mix_LoadWAV(assetPath);
    if (!game->uiNavigationSound) {
        SDL_Log("Failed to load ui nav sound: %s", Mix_GetError());
        goto fail;
    }

    if (!paths_resolve(assetPath, sizeof(assetPath), "resources/WonRace.WAV")) {
        SDL_Log("Failed to find resources/WonRace.WAV");
        goto fail;
    }
    game->victorySound = Mix_LoadWAV(assetPath);
    if (!game->victorySound) {
        SDL_Log("Failed to load victory sound: %s", Mix_GetError());
        goto fail;    
    }

    if (!paths_resolve(assetPath, sizeof(assetPath), "resources/LostRace.wav")) {
        SDL_Log("Failed to find resources/LostRace.wav");
        goto fail;
    }
    game->loseSound = Mix_LoadWAV(assetPath);
    if (!game->loseSound) {
        SDL_Log("Failed to load lose sound: %s", Mix_GetError());
        goto fail;
    }

    // Initialize Leaderboard fields
    game->leaderboard.entries = NULL;
    game->leaderboard.entryCount = 0;
    game->leaderboard.texture = NULL;
    game->leaderboard.isActive = false;
    game->showLeaderboard = false;

    // Initialize totalRaceTime
    game->totalRaceTime = 0.0;

    // Initialize the Animation fields
    game->trafficLightAnim.frameCount = 5;
    game->trafficLightAnim.frames = arena_alloc(&game->levelArena, sizeof(Sprite*) * 5, alignof(Sprite*));
    game->trafficLightAnim.frameDurations = arena_alloc(&game->levelArena, sizeof(double) * 5, alignof(double));

    // Attach the 5 frames
    game->trafficLightAnim.frames[0] = &SPRITE_LIGHT_FRAME1; 
    game->trafficLightAnim.frames[1] = &SPRITE_LIGHT_FRAME2;
    game->trafficLightAnim.frames[2] = &SPRITE_LIGHT_FRAME3;
    game->trafficLightAnim.frames[3] = &SPRITE_LIGHT_FRAME4;
    game->trafficLightAnim.frames[4] = &SPRITE_LIGHT_FRAME5;

    // Each frame 0.92s = total 4.6s
    for (int i = 0; i < 5; i++) {
        game->trafficLightAnim.frameDurations[i] = 0.92;
    }

    game->trafficLightAnim.currentFrame = 0;
    game->trafficLightAnim.currentTime  = 0.0;
    game->trafficLightAnim.active       = false;   // start inactive
    game->trafficLightAnim.finished     = false;
    game->trafficLightAnim.loop         = false;  // play once

    if (!paths_resolve(assetPath, sizeof(assetPath), "resources/opponents_sheet_v3.png")) {
        SDL_Log("Failed to find resources/opponents_sheet_v3.png");
        goto fail;
    }
    SDL_Texture* oppTex = IMG_LoadTexture(renderer, assetPath);
    if (!oppTex) {
        SDL_Log("Failed to load opponent sprites: %s", IMG_GetError());
        goto fail;
    }
    game->opponentSpritesheet = oppTex;

    // Reset road segments
    resetRoad(game);

    // Initialize additional game state variables
    game->lodCycleThreshold = (game->road.trackLength > 0.0) ? (game->road.trackLength / 3.0) : 1000.0;
    game->horizonY = 0.0;

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

    // Initialize AI Controller with opponents
    initAIController(&game->aiController, game, 5);

    // Init AI driver sprite to STRAIGHT version
    if (game->aiController.driverCount > 0) {
        game->aiController.drivers[0].sprite = &SPRITE_AI_STRAIGHT;
    }

    // Race win lose condition tracking
    game->totalLaps = 2;
    game->playerLapsCompleted = 0;
    game->raceFinished = false;

    // Collision flag
    game->inCollisionPush = false;
    game->collisionResetOccurredThisFrame = false;
    double horizonY = calculateHorizonY(game);
    for (int stateIndex = 0; stateIndex < game->slidingWindow->cycleStateCount; stateIndex++) {
        BackgroundCycleState *state = &game->slidingWindow->cycleStates[stateIndex];
        // For each of the three layers
        for (int i = 0; i < 3; i++) {
            // Compute the ideal y offset for layer
            float idealY = (float)horizonY - state->textureRects[i].h + state->overlapMargins[i];
            state->offsets[i].y = idealY;
        }
    }

    return true;

fail:
    cleanupGame(game);
    return false;
}

int handleEvent(Game* game, SDL_Event* event) {
    // UI Input Handling
    UIAction action = UI_ACTION_NONE;
    if (game->ui.currentState != UI_STATE_NONE) {
         action = handleUIInput(game, &game->ui, event);
         // Process action if it affects state
         switch(action) {
             case UI_ACTION_START_GAME:
                 // Reset game state for starting
                 resetGameForStart(game); // activate traffic light animation
                 setUIState(&game->ui, UI_STATE_NONE);
                 game->raceState = RACE_STATE_COUNTDOWN; // Start countdown
                 game->movementAllowed = false;
                 // Start countdown sound/animation
                 if (game->trafficLightSound) {
                     Mix_PlayMusic(game->trafficLightSound, 0); // Play once
                 }
                 break;
             case UI_ACTION_EXIT_GAME:
                 // Signal main loop to exit
                 return 0; // return 0 for main loop to stop
             case UI_ACTION_RESUME_GAME:
                 if (game->ui.currentState == UI_STATE_PAUSE_MENU) {
                    setUIState(&game->ui, UI_STATE_NONE);
                    game->raceState = RACE_STATE_RUNNING;
                    game->movementAllowed = true;
                    Mix_ResumeMusic(); // Resume paused music
                 }
                 break;
             case UI_ACTION_RESTART_GAME:
             case UI_ACTION_REPLAY_GAME:
                  resetGameForStart(game); // Resets and activates animation
                  setUIState(&game->ui, UI_STATE_NONE);
                  game->raceState = RACE_STATE_COUNTDOWN;
                  game->movementAllowed = false;
                  if (game->trafficLightSound) { // Play sound on restart/replay
                      Mix_PlayMusic(game->trafficLightSound, 0);
                  }
                  break;

             // to menu
             case UI_ACTION_EXIT_TO_MENU:
             case UI_ACTION_BACK_TO_MENU:
                  setUIState(&game->ui, UI_STATE_START_MENU); // To start menu UI
                  game->raceState = RACE_STATE_COUNTDOWN;
                  game->movementAllowed = false;
                  game->showLeaderboard = false;
                  game->raceFinished = false; // race not marked as finished
                  // deactivate traffic light animation
                  game->trafficLightAnim.active = false;
                  game->trafficLightAnim.finished = true; // Mark as finished
                  Mix_HaltMusic(); // Stop any ongoing game/countdown music
                  break;

             case UI_ACTION_NONE:
                // No action from UI this frame
                break;
         }
         return 1; // Continue running
    }

    // Game Input Handling when UI inactive
    // Only process keys if ui inactive and movement allowed
    if (game->ui.currentState == UI_STATE_NONE && game->movementAllowed) {
        if (event->type == SDL_KEYDOWN) {
            switch (event->key.keysym.sym) {
                case KEY_LEFT: case KEY_A: game->keyLeft = 1; break;
                case KEY_RIGHT: case KEY_D: game->keyRight = 1; break;
                case KEY_UP: case KEY_W: game->keyAccelerate = 1; break;
                case KEY_DOWN: case KEY_S: game->keyBrake = 1; break;
            }
        } else if (event->type == SDL_KEYUP) {
             switch (event->key.keysym.sym) {
                case KEY_LEFT: case KEY_A: game->keyLeft = 0; break;
                case KEY_RIGHT: case KEY_D: game->keyRight = 0; break;
                case KEY_UP: case KEY_W: game->keyAccelerate = 0; break;
                case KEY_DOWN: case KEY_S: game->keyBrake = 0; break;
            }
        }
    }

    // Global Input
     if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE) {
         if (game->ui.currentState == UI_STATE_NONE && game->raceState == RACE_STATE_RUNNING) {
             // Pause the game
             setUIState(&game->ui, UI_STATE_PAUSE_MENU);
             game->movementAllowed = false;
             Mix_PauseMusic();
         } else if (game->ui.currentState == UI_STATE_PAUSE_MENU) {
             // Unpause the game
             setUIState(&game->ui, UI_STATE_NONE);
             game->raceState = RACE_STATE_RUNNING;
             game->movementAllowed = true;
             Mix_ResumeMusic();
         } else if (game->showLeaderboard) {
              // If leaderboard is shown after race, ESC goes to main menu
              setUIState(&game->ui, UI_STATE_START_MENU); // start menu UI
              game->raceState = RACE_STATE_COUNTDOWN; // MENU state
              game->movementAllowed = false;
              game->showLeaderboard = false;
              game->raceFinished = false; // race marked as unfinished
              // Explicitly deactivate traffic light animation
              game->trafficLightAnim.active = false;
              game->trafficLightAnim.finished = true;
              //Mix_HaltMusic(); // Stop sounds
              Mix_HaltChannel(-1);
         }
         return 1;
     }
     return 1; // cont main loop
}

// Update the game state
void updateGame(Game* game, double dt) {
    arena_reset(&game->frameArena);
    game->collisionResetOccurredThisFrame = false;

    if (game->raceState == RACE_STATE_COUNTDOWN) {
        updateTrafficLightAnimation(game, dt);
        return;
    }

    if (!(game->movementAllowed && game->ui.currentState == UI_STATE_NONE))
        return;

    if (game->raceState == RACE_STATE_RUNNING) {
        game->totalRaceTime += dt;
        game->currentLapTime += dt;
    }

    double startPosition = game->position;

    Segment* playerSegment = findSegment(game, game->position + game->playerZ);
    if (!playerSegment) {
        SDL_Log("Error: Player segment not found.");
        return;
    }

    updateAIAndCollisions(game, dt, playerSegment);
    updatePlayerPhysics(game, dt, playerSegment);
    handleOffRoadCollisions(game, dt, playerSegment);
    updateLapLogic(game, startPosition);

    // Background Updates

    // Calculate projected horizon y value based on highest visible road point
    game->horizonY = calculateHorizonY(game);

    // Update background parallax scrolling only if no transition
    // Final parallax sync before transitions; uses updated segment/elevation to prevent visual pops
    updateBackgroundParallax(game, playerSegment, startPosition, dt);

    // Automatically trigger background transition logic when passing LOD threshold
    if (game->slidingWindow && !game->slidingWindow->transitioning && game->position >= game->lodCycleThreshold) {
        advanceToNextState(game, game->slidingWindow); // Initiates transition flags
    }

    // Handle visual scrolling effect during background transition
    updateBackgroundTransition(game, dt, startPosition);
}

// Render the game
void renderGame(Game* game) {
    // Render the main game scene e.g. background, road, sprites, player
    render(game); // Call main render func

    // Render UI on top if active
    if (game->ui.currentState != UI_STATE_NONE) {
        renderUI(game->renderer, &game->ui, game->width, game->height);
    }
    // Render Leaderboard Overlay if race finished
    else if (game->raceState == RACE_STATE_FINISHED && game->showLeaderboard) {
         if (game->leaderboard.texture && game->leaderboard.isActive) {
            SDL_RenderCopy(game->renderer, game->leaderboard.texture, NULL, &game->leaderboard.rect);
         }
         // render "Press ESC to return to Menu" text
         renderText(game->renderer, game->hudFont, "Press ESC for Main Menu",
                    game->width / 2, game->leaderboard.rect.y + game->leaderboard.rect.h + 20,
                    (SDL_Color){255,255,255,255}, true);
    }
    // Render HUD only during active race
    else if (game->raceState == RACE_STATE_RUNNING || game->raceState == RACE_STATE_COUNTDOWN) {
         renderHUD(game);
    }
}

/*
 * Frees all resources owned by Game (textures, fonts, mixer music/chunks, arenas, sliding window).
 * Cycle-state .texture pointers are non-owning aliases of game->background; clear them before
 * destroying the shared texture. Does not call SDL_Quit / TTF_Quit / IMG_Quit — main owns those.
 */
void cleanupGame(Game* game) {
    game->currentCycleState = NULL;
    game->targetCycleState = NULL;

    Mix_HaltMusic();
    Mix_HaltChannel(-1);

    if (game->slidingWindow) {
        freeBackgroundResources(game->slidingWindow);
    }

    if (game->background) {
        SDL_DestroyTexture(game->background);
        game->background = NULL;
    }
    if (game->spritesheet) {
        SDL_DestroyTexture(game->spritesheet);
        game->spritesheet = NULL;
    }
    if (game->opponentSpritesheet) {
        SDL_DestroyTexture(game->opponentSpritesheet);
        game->opponentSpritesheet = NULL;
    }
    if (game->animatedSpritesheet) {
        SDL_DestroyTexture(game->animatedSpritesheet);
        game->animatedSpritesheet = NULL;
    }

    if (game->slidingWindow) {
        free(game->slidingWindow);
        game->slidingWindow = NULL;
    }

    if (game->music) {
        Mix_FreeMusic(game->music);
        game->music = NULL;
    }
    if (game->trafficLightSound) {
        Mix_FreeMusic(game->trafficLightSound);
        game->trafficLightSound = NULL;
    }
    if (game->uiNavigationSound) {
        Mix_FreeChunk(game->uiNavigationSound);
        game->uiNavigationSound = NULL;
    }
    if (game->victorySound) {
        Mix_FreeChunk(game->victorySound);
        game->victorySound = NULL;
    }
    if (game->loseSound) {
        Mix_FreeChunk(game->loseSound);
        game->loseSound = NULL;
    }

    if (game->leaderboard.texture) {
        SDL_DestroyTexture(game->leaderboard.texture);
        game->leaderboard.texture = NULL;
    }

    if (game->hudFont) {
        TTF_CloseFont(game->hudFont);
        game->hudFont = NULL;
    }

    arena_reset(&game->levelArena);
    arena_reset(&game->frameArena);
    SDL_free(game->levelArenaStart);
    SDL_free(game->frameArenaStart);
    game->levelArenaStart = NULL;
    game->frameArenaStart = NULL;
}

double calculateHorizonY(Game* game) {
    double offset = 0; // negative values move this upward
    if (game->highestRoadY > 0) {
        return game->highestRoadY + offset;
    } else {
        return (double)game->height / 2.0;
    }
}