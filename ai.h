#ifndef AI_H
#define AI_H

#include <stdbool.h>
#include "constants.h"

struct Game;

typedef enum {
    AI_BEHAVIOR_NONE,
    AI_BEHAVIOR_WOBBLE,
    AI_BEHAVIOR_LANE_SWITCH,
    AI_BEHAVIOR_HOLD_LINE,
    AI_BEHAVIOR_RAMMING,
    AI_BEHAVIOR_SPEEDUP,
    AI_BEHAVIOR_SLOWDOWN
} AIBehaviorType;

// Structure for a single AI opponent (driver)
typedef struct {
    double z;                // Current position along the track (wrapped)
    double offset;           // Lateral offset from the road center, normalized to [-1,1]
    double speed;            // Current speed
    double targetSpeed;      // Speed the AI is trying to reach
    double targetLaneOffset; // Desired lateral offset
    double defaultLaneOffset; // Default lane offset
    double steering;         // Latest steering adjustment value
    double percent;          // Percent across the current segment (for rendering interpolation)
    
    // Lap timing and count (for race progress)
    int lapsCompleted;
    double lapTime;          // Time elapsed during the current lap
    double bestLapTime;      // Best (lowest) lap time recorded
    bool finished;           // Finished race flag

    double targetPushX;      // Target lateral push distance
    double elapsedPushTime;  // Time elapsed during push interpolation
    bool inCollision;       // Track ongoing collision state

    // pointer to the sprite for rendering the AI opponent
    const Sprite* sprite;
    double renderedY;        // Screen Y-coordinate where the AI was last rendered
    int lastCollisionType;    // COLLISION_NONE, COLLISION_FRONT, etc
    double lastPlayerX;        // For tracking lateral movement after rear collisions

    double totalRaceTime;
    int variant;

    double pushStartOffset;    // Initial offset when push begins
    double pushTargetOffset;   // Target offset after push
    double pushElapsed;        // Time elapsed during push
    double pushDuration;       // Total push duration
    bool isBeingPushed;        // Push animation flag

    bool isRamming;          // True when currently attempting to ram the player
    double ramTime;          // Time elapsed in the current ramming phase
    double ramCooldown;      // Cooldown timer until next ramming attempt

    int currentBehavior;
    double driftSpeed;
    int recovering;
    float aggressionFactor;

    float slowdownTime;         // Timer for how long behavior lasts
    float slowdownDuration;     // How long to hold the slowed speed
    float slowdownTargetSpeed;  // Temporary slower speed
    int slowdownPhase;          // 0 = decelerating, 1 = holding, 2 = accelerating

    float speedupTime;         // Timer for how long behavior lasts
    float speedupDuration;     // How long to hold the boosted speed
    float speedupTargetSpeed;  // Temporary boosted speed
    int speedupPhase;          // 0 = accelerating, 1 = holding, 2 = decelerating
    double behaviorElapsed;
    double initialBehaviorDelayTimer;
} AIDriver;

// Structure for the AI controller; holds the collection of AI drivers
typedef struct {
    AIDriver* drivers;     // Dynamic array of AI opponent structures
    int driverCount;       // How many AI drivers exist
    double catchupThreshold; // Fraction of track beyond which to boost speed
    double cornerSpeedFactor;  // Fraction of maxSpeed to use in a curve
    double maxAcceleration;    // Maximum acceleration rate for the AI
    double maxBraking;         // Maximum (absolute) braking deceleration
    double steeringSensitivity;// Factor controlling how fast the AI adjusts its offset
    double avoidanceStrength;  // Additional adjustment to avoid collisions with the player
    struct Game* game;         // Back pointer to the game
} AIController;

// Initialize the AI controller with a given opponent count
void initAIController(AIController* controller, struct Game* game, int numDrivers);

// Update all AI drivers in controller
void updateAIController(AIController* controller, double dt);

// Update an individual AI driver
void updateAIDriver(AIDriver* driver, double dt, struct Game* game, AIController* ctrl);

#endif // AI_H