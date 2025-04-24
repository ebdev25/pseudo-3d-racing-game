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

typedef enum {
    RACE_STATE_COUNTDOWN,
    RACE_STATE_RUNNING,
    RACE_STATE_FINISHED
} RaceState;

// Leaderboard Entry structure
typedef struct {
    char name[32];
    int lapsCompleted;
    double totalDistance; // laps * trackLength + current position
    double totalRaceTime;
    bool isPlayer;
} LeaderboardEntry;

// Leaderboard structure
typedef struct {
    LeaderboardEntry* entries;
    int entryCount;
    SDL_Texture* texture;
    SDL_Rect rect;
    bool isActive;
} Leaderboard;

// Game structure
typedef struct Game {
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
void initGame(Game* game, SDL_Renderer* renderer); // Initialize the game
int handleEvent(Game* game, SDL_Event* event);    // Handle input events
void updateGame(Game* game, double dt);            // Update game state
void renderGame(Game* game);                       // Render the game
void cleanupGame(Game* game);                      // Cleanup resources
double calculateHorizonY(Game *game);              // Calculate projected horizon line on screen
void generateLeaderboard(Game* game);              // Generate the leaderboard for end of game
#endif // GAME_H