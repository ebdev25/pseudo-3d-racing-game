#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "background.h"
#include "constants.h"
#include "road.h"
#include "level.h"
#include "ai.h"
#include "ui.h"
#include "mem_arena.h"
#include "leaderboard.h"

typedef enum {
    RACE_STATE_COUNTDOWN,
    RACE_STATE_RUNNING,
    RACE_STATE_FINISHED
} RaceState;

// Game structure
typedef struct Game {
    // Memory - per frame arena
    Arena frameArena; // per frame arena allocator
    void *frameArenaStart; // start of the big arena block
    size_t frameArenaCap; // its capacity

    // perm arena
    Arena levelArena;
    void *levelArenaStart;
    size_t levelArenaCap;

    RaceState raceState;      // current race state
    bool movementAllowed;     // player/AI move flag
    
    SDL_Renderer* renderer;     // SDL renderer
    double position;            // Player's position along the track
    double speed;               // Current player speed
    double playerX;             // Player's horizontal offset from road center
    double playerZ;             // Camera position along the Z-axis
    double cameraDepth;         // Camera depth for perspective projection
    double resolution;          // Screen resolution scaling factor
    int width;                  // Screen width
    int height;                 // Screen height

    // Input keys
    int keyLeft;                // Left movement key state
    int keyRight;               // Right movement key state
    int keyAccelerate;          // Accelerate key state
    int keyBrake;               // Brake key state

    // Sprites
    SDL_Texture* background;    // Background texture
    SDL_Texture* spritesheet;   // Spritesheet texture
    SDL_Texture* opponentSpritesheet;

    // Road and track parameters
    Road road;                  // Road structure containing segments
    int rumbleLength;           // Length of rumble strips
    double maxSpeed;            // Maximum player speed
    double accel;               // Acceleration rate
    double breaking;            // Braking rate
    double decel;               // Deceleration rate (natural)
    double offRoadDecel;        // Deceleration rate when off-road
    double offRoadLimit;        // Maximum speed when off-road
    double centrifugal;         // Centrifugal force for turning

    // Background LOD cycling
    int lodCycleThreshold;      // Distance threshold for LOD cycling

    // Parallax scrolling
    SlidingWindow* slidingWindow;    // Sliding window for managing LOD states
    BackgroundCycleState* currentCycleState; // Current cycle state
    BackgroundCycleState* targetCycleState;  // Target cycle state

    float storedOffsets[3];
    float storedOffsetsY[3];
    int transitionHappening;
    int transitionElapsed;

    // Camera and drawing parameters
    int lanes;                  // Number of road lanes
    int drawDistance;           // Number of segments drawn ahead of the player
    double highestRoadY;
    double lowestRoadY;
    double horizonY;               // Y-coordinate for the horizon

    // Timing parameters
    double currentLapTime;      // Current lap time
    double lastLapTime;         // Time for the last completed lap

    // Actual lap parameters
    int totalLaps;
    bool raceFinished;
    int playerLapsCompleted;

    // Collision flag
    bool inCollisionPush;
    double playerRenderedY;  // Screen Y-coordinate where the player was last rendered

    // Leaderboard fields (for end-of-race display)
    Leaderboard leaderboard;
    TTF_Font* hudFont; // Font for HUD elements
    double totalRaceTime; // Player's total race time

    // Audio
    Mix_Music* music;           // Background music
    LevelRoadData loadedRoadData;
    LevelSceneryData loadedScenery;

    // AI Opponents
    AIController aiController;

    // Animations
    SDL_Texture* animatedSpritesheet;
    Animation trafficLightAnim;
    Mix_Music* trafficLightSound;

    UI ui;
    Mix_Chunk* uiNavigationSound;
    Mix_Chunk* victorySound;
    Mix_Chunk* loseSound;
    bool showLeaderboard; // Flag to control leaderboard display
    bool collisionResetOccurredThisFrame; // flag to prevent lap increment on roadside sprite collision from player
} Game;

extern Game game;

// Function declarations
void resetGameForStart(Game* game);
bool initGame(Game* game, SDL_Renderer* renderer, const char* levelRelPath); /* NULL -> DEFAULT_LEVEL_REL_PATH */
int handleEvent(Game* game, SDL_Event* event);    // Handle input events
void updateGame(Game* game, double dt);            // Update game state
void renderGame(Game* game);                       // Render the game
void cleanupGame(Game* game); /* Game-owned GPU/audio/arenas only; not SDL_Quit/TTF_Quit/Mix (see main.c) */
double calculateHorizonY(Game *game);              // Calculate projected horizon line on screen
#endif // GAME_H