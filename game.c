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

    // Load HUD font
    game->hudFont = TTF_OpenFont("../resources/PressStart2P-Regular.ttf", 24);
    if (!game->hudFont) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        TTF_Quit();
        exit(1);
    }

    // Initialize UI system AFTER font is loaded
    initUI(&game->ui, game->hudFont); // Pass the loaded font
    setUIState(&game->ui, UI_STATE_START_MENU); // Start with the main menu

    // Load UI navigation sound
    game->uiNavigationSound = Mix_LoadWAV("../resources/uiNavSound.wav");
    if (!game->uiNavigationSound) {
        SDL_Log("Failed to load ui nav sound: %s", Mix_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
    }

    game->victorySound = Mix_LoadWAV("../resources/WonRace.WAV");
    if (!game->victorySound) {
        SDL_Log("Failed to load victory sound: %s", Mix_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);    
    }

    game->loseSound = Mix_LoadWAV("../resources/LostRace.wav");
    if (!game->loseSound) {
        SDL_Log("Failed to load lose sound: %s", Mix_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        exit(1);
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
    game->trafficLightAnim.frames = (const Sprite**)malloc(sizeof(Sprite*) * 5);
    game->trafficLightAnim.frameDurations = (double*)malloc(sizeof(double) * 5);

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

    // Load opponent spritesheet
    SDL_Texture* oppTex = IMG_LoadTexture(renderer, "/home/horatiobelter/Desktop/Programming/Pseudo 3D Racer/resources/opponents_sheet_v2.png");
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
    game->collisionResetOccurredThisFrame = false;
    // Update Countdown Animation
    // update traffic light animation during countdown phase
    if (game->raceState == RACE_STATE_COUNTDOWN) {
        updateTrafficLightAnimation(game, dt);
    }

    // Main Game Logic, Physics, AI, Player Control
    // Only run core game updates if movement allowed and UI not active
    if (game->movementAllowed && game->ui.currentState == UI_STATE_NONE) {

        double playerW = SPRITE_PLAYER_STRAIGHT.w * SPRITE_SCALE;

        // Percentage of max speed that player is currently moving
        double speedPercent = (game->maxSpeed > 0.0) ? (game->speed / game->maxSpeed) : 0.0; // Avoid div by zero

        // Horizontal movement increment based on time step and speed
        double dx = dt * 2.0 * speedPercent;

        // Store start pos to calculate distance moved for lap timing
        double startPosition = game->position;

        // Update total race time only when race running
        if(game->raceState == RACE_STATE_RUNNING) {
             game->totalRaceTime += dt;
             game->currentLapTime += dt;
        }


        // Find current player seg
        Segment* playerSegment = findSegment(game, game->position + game->playerZ);
        if (playerSegment == NULL) {
            SDL_Log("Error: Player segment not found in updateGame.\n");
            return;
        }

        // Update AI opponents
        updateAIController(&game->aiController, dt);

        // Check for AI driver collisions
        checkAIDriverCollisions(game);

        // Check for player AI collisions
        checkPlayerAIOpponentCollisions(game);

        // Process AI colliding w/ player
        {
            Segment* seg = findSegment(game, game->position + game->playerZ); // find seg again, pos might have changed significantly
            if (seg) {
                for (int i = 0; i < game->aiController.driverCount; i++) {
                    AIDriver* driver = &game->aiController.drivers[i];
                     // AI driver in same seg to interact
                     Segment* aiSeg = findSegment(game, driver->z);
                     if(aiSeg != seg) continue; // Skip AI in different segments

                    double aiW = (driver->sprite) ? driver->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;
                    // Check for lateral overlap if the AI in ramming state
                    if (overlap(game->playerX, playerW, driver->offset, aiW, 1.0) && driver->isRamming) {
                        // Apply push
                        double pushForce = 0.1 * dt * 60; // Scale force with dt
                        game->playerX += (driver->offset < game->playerX) ? pushForce : -pushForce;
                    }
                }
            }
        }

        // Player Physics and Control

        // Move player pos forward based on current speed
        game->position = increase(game->position, dt * game->speed, game->road.trackLength);

        // Handle player input for left/right movement (only if not being pushed by collision)
        if (!game->inCollisionPush) {
            if (game->keyLeft)
                game->playerX -= dx;
            if (game->keyRight)
                game->playerX += dx;
        }

        // Apply centrifugal force based on road curve
        game->playerX -= dx * speedPercent * playerSegment->curve * game->centrifugal;

        // Handle acceleration and deceleration based on input
        if (game->keyAccelerate)
            game->speed = accelerate(game->speed, game->accel, dt);
        else if (game->keyBrake)
            game->speed = accelerate(game->speed, game->breaking, dt);
        else
            game->speed = accelerate(game->speed, game->decel, dt); // Natural deceleration

        // Handle player going off-road
        if ((game->playerX < -1.0) || (game->playerX > 1.0)) {
            // Slow down player if exceeding off-road speed limit
            if (game->speed > game->offRoadLimit)
                game->speed = accelerate(game->speed, game->offRoadDecel, dt);

            // Check for collision with roadside sprites (e.g., trees, billboards)
             for (int n = 0; n < playerSegment->spriteCount; n++) {
                 SpriteInstance* sprite = &playerSegment->sprites[n];
                 double spriteW = sprite->source->w * SPRITE_SCALE;
                 // Approximate sprite center for collision check
                 double spriteCenterOffset = sprite->offset + ((sprite->offset > 0.0) ? (spriteW / 2.0) : (-spriteW / 2.0));

                if (overlap(game->playerX, playerW, sprite->offset + spriteW / 2 * (sprite->offset > 0 ? 1 : -1), spriteW, 1.0)) {
                    game->speed = game->maxSpeed / 5.0; 
                    // prevent player moving forward
                    game->position = increase(playerSegment->p1.world.z, -game->playerZ, game->road.trackLength); 

                    game->collisionResetOccurredThisFrame = true; // prevent lap increment

                    SDL_Log("Collision with roadside sprite detected. Speed reduced and position reset.\n");
                    break; 
                }
             }
        }

        // Clamp player's horizontal position and speed to limits
        game->playerX = limit(game->playerX, -3.0, 3.0); // Limit X to prevent going too far off-screen
        game->speed = limit(game->speed, 0.0, game->maxSpeed); // Ensure speed doesn't exceed max or go below zero

        // Lap Timing and Race Finish Check
        // Check if the player crossed the start/finish line (position wrapped around)
        if (!game->collisionResetOccurredThisFrame && game->position < startPosition && startPosition > game->playerZ) { // Wrapped around
             if(game->currentLapTime > 1.0) { // Debounce crossing line immediately
                game->playerLapsCompleted++;
                game->lastLapTime = game->currentLapTime;
                game->currentLapTime = 0.0; // Reset timer for new lap
                SDL_Log("Player completed lap #%d in %.2f seconds",
                        game->playerLapsCompleted, game->lastLapTime);

                // Check if player finished required number laps
                if (game->playerLapsCompleted >= game->totalLaps && !game->raceFinished) {
                    SDL_Log("Player has finished the race!");
                    game->raceState = RACE_STATE_FINISHED;
                    game->movementAllowed = false; // Stop player control
                    game->raceFinished = true;
                    Mix_HaltMusic(); // Stop race music
                    generateLeaderboard(game); // Calculate and prepare leaderboard data
                    game->showLeaderboard = true; // Set flag to display leaderboard in renderGame
                    bool playerWon = false;
                     // Check if leaderboard was successfully generated and has entries
                     if (game->leaderboard.isActive && game->leaderboard.entryCount > 0 && game->leaderboard.entries) {
                         // Player won if the first entry in the sorted list is the player
                         if (game->leaderboard.entries[0].isPlayer) {
                             playerWon = true;
                         }
                     } else {
                         SDL_Log("Warning: Leaderboard not active or empty when checking win condition for sound.");
                         // Default to playing lose sound if leaderboard failed
                         playerWon = false;
                     }

                     // Play sound effect once
                     if (playerWon) {
                         if (game->victorySound) {
                             Mix_PlayChannel(-1, game->victorySound, 0); // Play victory sound
                         }
                     } else {
                         if (game->loseSound) {
                             Mix_PlayChannel(-1, game->loseSound, 0); // Play lose sound
                         }
                     }
                }
                 // Reset LOD cycle threshold relative to the start line for the new lap
                 game->lodCycleThreshold = game->road.trackLength / 3.0; // Reset to first third
             }
        }


        // Background Updates

        // Calculate projected horizon y value based on highest visible road point
        game->horizonY = calculateHorizonY(game);

        // Update background parallax scrolling only if no transition
        if (!game->transitionHappening && game->slidingWindow && game->currentCycleState) {
             double elevation = calculateElevation(playerSegment, game->position);
             updateParallax(game, game->slidingWindow, game->currentCycleState, game->speed, playerSegment->curve, elevation,
                           game->position, startPosition, dt, game->horizonY, game->transitionHappening);
        }

        if (!game->transitionHappening && game->slidingWindow && game->currentCycleState) {
            Segment* currentSegmentForParallax = findSegment(game, game->position + game->playerZ);
            if (currentSegmentForParallax) { // Check if segment is valid
                double elevation = calculateElevation(currentSegmentForParallax, game->position);
                double previousPosition = startPosition;
                updateParallax(game, game->slidingWindow, game->currentCycleState, game->speed, currentSegmentForParallax->curve, elevation,
                               game->position, previousPosition, dt, game->horizonY, false);
            } else {
                 SDL_Log("Warning: Player segment null before pre-transition parallax update.");
            }
        }

        // Automatically trigger background transition logic when passing LOD threshold
        if (game->slidingWindow && !game->slidingWindow->transitioning && game->position >= game->lodCycleThreshold) {
             advanceToNextState(game, game->slidingWindow); // Initiates transition flags
             // Update the threshold for next transition point
             game->lodCycleThreshold += (game->road.trackLength / 3.0);
             // Wrap threshold if exceeds track length
             if (game->lodCycleThreshold > game->road.trackLength) {
                 game->lodCycleThreshold -= game->road.trackLength;
             }
        }

         // Handle visual scrolling effect during background transition
         if (game->transitionHappening && game->slidingWindow && game->currentCycleState) {
            // Update transition color interpolation progress
             double distanceIntoTransition = increase(game->position, -game->slidingWindow->transitionStartPosition, game->road.trackLength);
             game->slidingWindow->transitionProgress = limit(distanceIntoTransition / game->slidingWindow->transitionDistance, 0.0, 1.0);
             updateTransitionColors(game, game->slidingWindow, dt); // Update interpolated colors

             // Calculate how much background scroll needed based on player movement
             double movementSinceLastFrame = increase(game->position, -startPosition, game->road.trackLength);
             double backgroundScrollSpeed = movementSinceLastFrame * BACKGROUND_SCROLL_SCALE;

             // Scroll outgoing current state layers leftwards
             bool allCurrentLayersOffScreen = true;
             for (int i = 0; i < 3; i++) {
                 game->currentCycleState->offsets[i].x -= (int)backgroundScrollSpeed;

                 // Check if layer fully off-screen
                 int texW = game->currentCycleState->textureRects[i].w;
                 int extraW = game->currentCycleState->extraSlice[i]; // Width of potential second visible part
                 int effectiveWidth = texW + extraW; // Total width to scroll off

                 if (game->currentCycleState->offsets[i].x > -effectiveWidth) {
                     allCurrentLayersOffScreen = false; // Still visible
                 }
                 // Update Y position dynamically based on horizon
                 float targetY = (float)game->horizonY - game->currentCycleState->textureRects[i].h + game->currentCycleState->overlapMargins[i];
                 game->currentCycleState->offsets[i].y = lerp_float(game->currentCycleState->offsets[i].y, targetY, 0.1f); // Smooth Y adjustment
             }

            // If current state fully off-screen, scroll incoming target state
            if (allCurrentLayersOffScreen && game->targetCycleState) {
                 bool targetFullyOnScreen = true;
                 for (int i = 0; i < 3; i++) {
                     game->targetCycleState->offsets[i].x -= (int)backgroundScrollSpeed; // Scroll left

                     // Update Y position dynamically based on horizon
                     float targetY = (float)game->horizonY - game->targetCycleState->textureRects[i].h + game->targetCycleState->overlapMargins[i];
                     game->targetCycleState->offsets[i].y = lerp_float(game->targetCycleState->offsets[i].y, targetY, 0.1f); // Smooth Y adjustment

                     // Check if layer of target state fully on screen
                     if (game->targetCycleState->offsets[i].x > 0) { // If left edge still right of screen edge
                         targetFullyOnScreen = false;
                     }
                 }

                 // If all layers of target state are fully visible
                 if (targetFullyOnScreen) {
                     // Finalize transition
                     game->slidingWindow->currentCycleStateIndex = game->slidingWindow->targetCycleStateIndex;
                     game->currentCycleState = &game->slidingWindow->cycleStates[game->slidingWindow->currentCycleStateIndex];
                     // Determine next target index (wrapping around)
                     game->slidingWindow->targetCycleStateIndex = (game->slidingWindow->currentCycleStateIndex + 1) % game->slidingWindow->cycleStateCount;
                     if (game->slidingWindow->cycleStateCount > 1) {
                         game->targetCycleState = &game->slidingWindow->cycleStates[game->slidingWindow->targetCycleStateIndex];
                     } else {
                         game->targetCycleState = NULL; // No next target if only one state
                     }

                     // Reset transition flags
                     game->transitionHappening = 0;
                     game->slidingWindow->transitioning = false;
                     game->slidingWindow->transitionProgress = 0.0f; // Reset progress

                     SDL_Log("Background transition complete! Current state: %d\n", game->slidingWindow->currentCycleStateIndex);
                 }
             }
        }
    }
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
        }
        free(game->road.segments);
    }

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
        // free the main array
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

    if (game->leaderboard.entries) {
        free(game->leaderboard.entries);
        game->leaderboard.entries = NULL;
        game->leaderboard.entryCount = 0;
    }

    if (game->leaderboard.texture) {
        SDL_DestroyTexture(game->leaderboard.texture);
        game->leaderboard.texture = NULL;
    }

    if (game->hudFont) {
        TTF_CloseFont(game->hudFont);
        game->hudFont = NULL;
    }

    if (game->hudFont) {
        TTF_CloseFont(game->hudFont);
        game->hudFont = NULL;
    }

    // Quit SDL_ttf and SDL_mixer
    TTF_Quit();
    Mix_CloseAudio();
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

// Generate the leaderboard when race ends
void generateLeaderboard(Game* game) {
    // Validate AI Controller State
    if (!game->aiController.drivers && game->aiController.driverCount != 0) {
         SDL_Log("Error: AI driver array is NULL but count is %d. Resetting count.", game->aiController.driverCount);
         game->aiController.driverCount = 0; // Attempt recovery
    }
    if (game->aiController.driverCount < 0) {
        SDL_Log("Error: Negative AI driver count (%d). Resetting count.", game->aiController.driverCount);
         game->aiController.driverCount = 0; // Attempt recovery
    }

    SDL_Log("DEBUG PRE-FREE - entries: %p, driverCount: %d", 
            (void*)game->leaderboard.entries, 
            game->aiController.driverCount);

    // Free any previous leaderboard entries safely
    if (game->leaderboard.entries) {
        free(game->leaderboard.entries);
        game->leaderboard.entries = NULL; // Nullify pointer after freeing
    }
    game->leaderboard.entryCount = 0; // Reset entry count

    int numRacers = 1 + game->aiController.driverCount;
    // numRacers Validity
    if (numRacers <= 0) { // Handle integer overflow or negative driverCount case
         SDL_Log("Error: Calculated numRacers (%d) is invalid. Cannot generate leaderboard.", numRacers);
         game->leaderboard.isActive = false;
         return;
    }


    // Allocate and Initialize Leaderboard Memory
    game->leaderboard.entries = malloc(numRacers * sizeof(LeaderboardEntry));
    if (!game->leaderboard.entries) {
        SDL_Log("Failed to allocate memory for %d leaderboard entries.", numRacers);
        game->leaderboard.isActive = false; // Ensure leaderboard unused
        return; // Exit if allocation failed
    }
    // Initialize allocated memory
    memset(game->leaderboard.entries, 0, numRacers * sizeof(LeaderboardEntry));
    game->leaderboard.entryCount = numRacers; // Set the count after successful allocation

    // Determine if the player finished first
    bool playerFinishedFirst = true;
    for (int i = 0; i < game->aiController.driverCount; i++) {
        // Check pointer validity within loop
        if (!game->aiController.drivers) {
             SDL_Log("Error: AI drivers became NULL during playerFinishedFirst check.");
             playerFinishedFirst = false; // Assume player not first if data is bad
             break;
        }
        AIDriver* ai = &game->aiController.drivers[i];
        // Basic check on AI data used in comparison
        if (!ai || isnan(ai->totalRaceTime)) {
             SDL_Log("Warning: Invalid AI data encountered during playerFinishedFirst check for AI %d.", i);
             continue; // Skip any potentially corrupt AI
        }
        if (ai->finished && ai->totalRaceTime < game->totalRaceTime) {
            playerFinishedFirst = false;
            break;
        }
    }

    // AI Time Estimation block
    if (playerFinishedFirst) {
        // Check for drivers pointer validity before loop
        if (!game->aiController.drivers) {
            SDL_Log("Error: AI drivers pointer is NULL before estimation loop.");
        } else {
            for (int i = 0; i < game->aiController.driverCount; i++) {
                // Checks for each AI driver
                if (i >= game->aiController.driverCount) { // Bounds check
                    SDL_Log("Error: Index i (%d) out of bounds for AI drivers (%d)", i, game->aiController.driverCount);
                    break; // Stop loop if index is invalid
                }
                AIDriver* ai = &game->aiController.drivers[i];

                if (!ai) { // Check individual AI pointer
                    SDL_Log("Error: AI driver pointer at index %d is NULL during estimation.", i);
                    continue; // Skip potentially corrupt AI
                }

                // Skip if AI driver data is invalid/AI hasn't started/finished meaningfully
                if (ai->finished || ai->z <= 0.0 || isnan(ai->z) || isnan(ai->speed) || isnan(ai->totalRaceTime)) {
                    SDL_Log("Skipping estimation for AI %d: Finished or Invalid data (z:%.2f, speed:%.2f, time:%.2f)", i, ai->z, ai->speed, ai->totalRaceTime);
                    // Ensure existing time is sane if finished, otherwise fallback to set value
                    if(isnan(ai->totalRaceTime) || isinf(ai->totalRaceTime) || ai->totalRaceTime < 0.0) {
                        ai->totalRaceTime = 99999.0;
                    }
                    continue;
                }

                // Compute and validate remaining distance, ensure valid track length
                if (isnan(game->road.trackLength) || game->road.trackLength <= 0) {
                        SDL_Log("Warning: Invalid track length (%.2f) during estimation.", game->road.trackLength);
                        ai->totalRaceTime = 99999.0; // Assign fallback
                        continue;
                }
                double remainingDistance = game->road.trackLength - ai->z; // remaining distance on current lap
                if (remainingDistance < 0.0) { // wrap around case
                    remainingDistance += game->road.trackLength;
                }
                // validation for calculated remaining distance
                if (isnan(remainingDistance) || isinf(remainingDistance) || remainingDistance < 0.0) {
                    SDL_Log("Skipping AI %d: Invalid remaining distance calculated (%.2f)", i, remainingDistance);
                    ai->totalRaceTime = 99999.0; // Assign fallback
                    continue;
                }

                // Use minimum effective speed
                double effectiveSpeed = (ai->speed > 0.1 && !isnan(ai->speed) && !isinf(ai->speed)) ? ai->speed : 0.1;

                // Estimate remaining time and check result
                if (effectiveSpeed <= 0) { // Explicit check for zero/negative speed
                        SDL_Log("Skipping AI %d: Invalid effective speed (%.2f)", i, effectiveSpeed);
                        ai->totalRaceTime = 99999.0; // Assign fallback
                        continue;
                }
                double estimatedTimeRemaining = remainingDistance / effectiveSpeed;
                if (isnan(estimatedTimeRemaining) || isinf(estimatedTimeRemaining) || estimatedTimeRemaining < 0.0) {
                    SDL_Log("Skipping AI %d: Invalid estimated time (%.2f / %.2f = %.2f)", i, remainingDistance, effectiveSpeed, estimatedTimeRemaining);
                    ai->totalRaceTime = 99999.0; // Assign fallback
                    continue;
                }

                // Apply fudge factor and re-validate
                float min_fudge = 0.5f;
                float max_fudge = 0.8f;
                float random_zero_one = (float)rand() / (float)RAND_MAX;
                double randomFudgeFactor = min_fudge + random_zero_one * (max_fudge - min_fudge);
                estimatedTimeRemaining *= randomFudgeFactor;
                if (isnan(estimatedTimeRemaining) || isinf(estimatedTimeRemaining) || estimatedTimeRemaining < 0.0) {
                    SDL_Log("Skipping AI %d: Invalid time after fudge factor (%.2f)", i, estimatedTimeRemaining);
                    ai->totalRaceTime = 99999.0; // Assign fallback
                    continue;
                }

                // Add to totalRaceTime and validate again
                double previousTime = ai->totalRaceTime; // Store previous time for checking
                ai->totalRaceTime += estimatedTimeRemaining;
                if (isnan(ai->totalRaceTime) || isinf(ai->totalRaceTime) || ai->totalRaceTime < 0.0) {
                    SDL_Log("AI %d totalRaceTime became invalid after estimation (%.2f + %.2f = %.2f)! Resetting.", i, previousTime, estimatedTimeRemaining, ai->totalRaceTime);
                    ai->totalRaceTime = 99999.0; // fallback
                }
            }
        }
    }

    // Populate entries, checking bounds and pointers
    if (game->leaderboard.entries) { // Ensure entries pointer is still valid

        // Player entry (index 0)
        if (game->leaderboard.entryCount > 0) { // Check if space exists
            game->leaderboard.entries[0].isPlayer = true;
            game->leaderboard.entries[0].lapsCompleted = game->playerLapsCompleted;
            // check for potentially invalid game state values
            if (isnan(game->position) || isnan(game->totalRaceTime) || isinf(game->totalRaceTime) || game->totalRaceTime < 0.0) {
                SDL_Log("Error: Player position or totalRaceTime is invalid!");
                game->totalRaceTime = 99999.0; // fallback time
            }
            if(isnan(game->road.trackLength) || game->road.trackLength < 0) {
                SDL_Log("Error: Invalid trackLength for player distance calculation!");
                game->leaderboard.entries[0].totalDistance = 0; // Fallback distance
            } else {
                game->leaderboard.entries[0].totalDistance = game->playerLapsCompleted * game->road.trackLength + game->position;
            }
            game->leaderboard.entries[0].totalRaceTime = game->totalRaceTime;
            strncpy(game->leaderboard.entries[0].name, "Player", 31);
            game->leaderboard.entries[0].name[31] = '\0'; // Ensure null termination
        } else {
            SDL_Log("Error: Not enough space allocated for player leaderboard entry.");
        }


        // AI entries
        if (game->aiController.drivers) { // Check AI drivers pointer
            for (int i = 0; i < game->aiController.driverCount; i++) {
                int entryIndex = i + 1;
                // Check index validity against allocated count
                if (entryIndex >= game->leaderboard.entryCount) {
                    SDL_Log("Error: AI entry index (%d) exceeds allocated size (%d). Skipping.", entryIndex, game->leaderboard.entryCount);
                    continue; // Prevent writing out of bounds
                }

                // Check individual AI pointer
                if (i >= game->aiController.driverCount) { // Bounds check
                    SDL_Log("Error: Index i (%d) out of bounds for AI drivers (%d) in population loop", i, game->aiController.driverCount);
                    break;
                }
                AIDriver* ai = &game->aiController.drivers[i];
                if (!ai) {
                    SDL_Log("Error: AI driver pointer at index %d is NULL during population.", i);
                    // Fill leaderboard slot with placeholder data
                    game->leaderboard.entries[entryIndex].totalRaceTime = 99999.0;
                    strncpy(game->leaderboard.entries[entryIndex].name, "AI Error", 31);
                    game->leaderboard.entries[entryIndex].name[31] = '\0';
                    game->leaderboard.entries[entryIndex].isPlayer = false;
                    continue;
                }

                game->leaderboard.entries[entryIndex].isPlayer = false;
                game->leaderboard.entries[entryIndex].lapsCompleted = ai->lapsCompleted;
                // Check AI data validity before using
                if (isnan(ai->z) || isnan(ai->totalRaceTime) || isinf(ai->totalRaceTime) || ai->totalRaceTime < 0.0 || isnan(game->road.trackLength) || game->road.trackLength < 0) {
                    SDL_Log("Warning: Invalid data for AI %d during population (z:%.2f, time:%.2f, track:%.2f). Using fallback time/dist.", i, ai->z, ai->totalRaceTime, game->road.trackLength);
                    game->leaderboard.entries[entryIndex].totalDistance = ai->lapsCompleted * ((!isnan(game->road.trackLength) && game->road.trackLength > 0) ? game->road.trackLength : 0); // Estimate distance
                    game->leaderboard.entries[entryIndex].totalRaceTime = 99999.0; // fallback time
                } else {
                    game->leaderboard.entries[entryIndex].totalDistance = ai->lapsCompleted * game->road.trackLength + ai->z;
                    game->leaderboard.entries[entryIndex].totalRaceTime = ai->totalRaceTime;
                }
                snprintf(game->leaderboard.entries[entryIndex].name, 32, "AI %d", i+1);
            }
        } else {
            SDL_Log("Error: AI drivers pointer NULL during entry population.");
            // fill remaining slots with error messages if required
            for (int i = 0; i < game->aiController.driverCount; ++i) {
                int entryIndex = i + 1;
                if (entryIndex < game->leaderboard.entryCount) {
                        game->leaderboard.entries[entryIndex].totalRaceTime = 99999.0;
                        strncpy(game->leaderboard.entries[entryIndex].name, "AI Error", 31);
                        game->leaderboard.entries[entryIndex].name[31] = '\0';
                        game->leaderboard.entries[entryIndex].isPlayer = false;
                }
            }
        }
    } else {
        SDL_Log("Error: Leaderboard entries pointer NULL before population.");
        return;
    }


    // Sanitize Times Before Sorting
    if (game->leaderboard.entries) { // Check again before access
        for (int i = 0; i < game->leaderboard.entryCount; i++) { // Use stored count
            if (i >= game->leaderboard.entryCount) break; // Extra bounds safety
            LeaderboardEntry* entry = &game->leaderboard.entries[i];
            if (isnan(entry->totalRaceTime) || isinf(entry->totalRaceTime) || entry->totalRaceTime < 0.0) {
                SDL_Log("Sanitizing entry %d: Invalid race time (%.2f). Fixing to fallback.", i, entry->totalRaceTime);
                entry->totalRaceTime = 99999.0; // fallback
            }
            // clamp large times too up to 10 hours
            else if (entry->totalRaceTime > 36000.0) {
                SDL_Log("Sanitizing entry %d: Clamping excessive race time (%.2f).", i, entry->totalRaceTime);
                entry->totalRaceTime = 99999.0;
            }
        }
    }

    // Sort Leaderboard Entries Safely
    if (game->leaderboard.entryCount > 0 && game->leaderboard.entries != NULL) {
        qsort(game->leaderboard.entries, game->leaderboard.entryCount, sizeof(LeaderboardEntry), compareLeaderboardEntries);
    } else {
        SDL_Log("Warning: Skipping qsort due to entryCount=%d or entries=NULL", game->leaderboard.entryCount);
    }

    // Create Leaderboard Texture
    if (game->leaderboard.texture) {
        SDL_DestroyTexture(game->leaderboard.texture);
        game->leaderboard.texture = NULL;
    }

    // Check font validity
    if (!game->hudFont) {
        SDL_Log("Error: HUD Font is NULL. Cannot create leaderboard texture.");
        game->leaderboard.isActive = false;
        return; // Cannot proceed without font
    }

    const int padding = 20;
    const int entryHeight = 30;
    const int entrySpacing = 5;
    int texWidth = 400;

    // Calculate height based on num of valid entries
    int validEntryCount = 0;
    if (game->leaderboard.entries){
        for(int i = 0; i < game->leaderboard.entryCount; ++i) {
            // consistent validity check
            if (i >= game->leaderboard.entryCount) break; // Bounds check
            LeaderboardEntry* entry = &game->leaderboard.entries[i];
            if (!isnan(entry->totalRaceTime) && !isinf(entry->totalRaceTime) && entry->totalRaceTime >= 0.0 && entry->totalRaceTime < 90000.0) { // Check for times within limit
                validEntryCount++;
            }
        }
    } else {
        SDL_Log("Error: Leaderboard entries NULL before texture dimension calculation.");
        return;
    }

    if (validEntryCount <= 0) {
        SDL_Log("Warning: No valid leaderboard entries to render texture for.");
        game->leaderboard.isActive = false;
        return; // avoid creating empty texture
    }

    // Calculate height based on valid entries
    int texHeight = padding * 2 + (entryHeight + entrySpacing) * validEntryCount - entrySpacing; // Subtract last spacing

    // Create surface
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, texWidth, texHeight, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        SDL_Log("Failed to create leaderboard surface (%dx%d): %s", texWidth, texHeight, SDL_GetError());
        game->leaderboard.isActive = false; // start inactive
        // free entries
        if (game->leaderboard.entries) {
            free(game->leaderboard.entries);
            game->leaderboard.entries = NULL;
            game->leaderboard.entryCount = 0;
        }
        return; // failure
    }

    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 150));

    SDL_Color textColor = {255, 255, 255, 255};
    int y = padding;
    int drawnEntries = 0; // Counter for drawn entries

    if(game->leaderboard.entries){ // Check again before loop
        for (int i = 0; i < game->leaderboard.entryCount && drawnEntries < validEntryCount; i++) { // Use stored count, limit by validEntryCount
            LeaderboardEntry* entry = &game->leaderboard.entries[i];

             if (isnan(entry->totalRaceTime) || entry->totalRaceTime < 0.0 || entry->totalRaceTime > 1e7) { // Increased upper bound
                SDL_Log("Skipping drawing corrupted leaderboard entry %d (time: %f)", i, entry->totalRaceTime);
                continue; // Skip rendering this entry
            }

            char posText[16], timeText[32];
            snprintf(posText, sizeof(posText), "%d", drawnEntries + 1); // Pos based on drawn entries
            snprintf(timeText, sizeof(timeText), "%.2fs", entry->totalRaceTime);

            // Render Text Surfaces with Checks
            SDL_Surface* posSurf = TTF_RenderText_Blended(game->hudFont, posText, textColor);
            SDL_Surface* nameSurf = TTF_RenderText_Blended(game->hudFont, entry->name, textColor);
            SDL_Surface* timeSurf = TTF_RenderText_Blended(game->hudFont, timeText, textColor);

            if (!posSurf || !nameSurf || !timeSurf) {
                SDL_Log("Failed to render text for entry %d: %s", i, TTF_GetError());
                SDL_FreeSurface(posSurf); // Free any that succeeded
                SDL_FreeSurface(nameSurf);
                SDL_FreeSurface(timeSurf);
                continue; // Skip entry if any part failed
            }

            // Blit Surfaces
            SDL_Rect posRect = {padding, y, posSurf->w, posSurf->h};
            SDL_BlitSurface(posSurf, NULL, surface, &posRect);

            SDL_Rect nameRect = {padding + 50, y, nameSurf->w, nameSurf->h};
            SDL_BlitSurface(nameSurf, NULL, surface, &nameRect);

            SDL_Rect timeRect = {texWidth - padding - timeSurf->w, y, timeSurf->w, timeSurf->h};
            SDL_BlitSurface(timeSurf, NULL, surface, &timeRect);

            SDL_FreeSurface(posSurf);
            SDL_FreeSurface(nameSurf);
            SDL_FreeSurface(timeSurf);

            y += entryHeight + entrySpacing;
            drawnEntries++; // Increment counter for drawn entries
        }
    } else {
         SDL_Log("Error: Leaderboard entries NULL before rendering loop.");
    }

    // Create Final Texture
    if (drawnEntries > 0) { // Only create texture if something drawn successfully
        game->leaderboard.texture = SDL_CreateTextureFromSurface(game->renderer, surface);
        if (!game->leaderboard.texture) {
            SDL_Log("Failed to create leaderboard texture from surface: %s", SDL_GetError());
            game->leaderboard.isActive = false;
        } else {
            SDL_SetTextureBlendMode(game->leaderboard.texture, SDL_BLENDMODE_BLEND);
             game->leaderboard.rect.x = (game->width - texWidth) / 2;
             game->leaderboard.rect.y = (game->height - texHeight) / 2; // Use calculated texHeight
             game->leaderboard.rect.w = texWidth;
             game->leaderboard.rect.h = texHeight;
             game->leaderboard.isActive = true;
             SDL_Log("Leaderboard texture created (%dx%d) with %d valid entries.", texWidth, texHeight, drawnEntries);
        }
    } else {
         SDL_Log("No valid entries were drawn, leaderboard inactive.");
         game->leaderboard.isActive = false;
    }

    SDL_FreeSurface(surface); // Free surface regardless
}