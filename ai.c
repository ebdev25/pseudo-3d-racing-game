#include "ai.h"
#include "game.h"
#include "util.h"
#include "constants.h"
#include "road.h"
#include "collision.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define AI_RAM_AGGRESSION_FACTOR 0.9  // lateral aggression
#define AI_RAM_DISTANCE_THRESHOLD (SEGMENT_LENGTH * 0.75)  // Allow ramming from further away
#define AI_RAM_PERSISTENCE_TIME 2.5  // AI will continue attacking for 1.5 seconds
#define AI_RAM_ATTACK_COOLDOWN 1.5  // AI waits this long before attacking again

void aiAggressiveSteer(AIDriver* driver, double dt, struct Game* game, AIController* ctrl) {
    // Check if the player's segment is valid.
    Segment* playerSeg = findSegment(game, game->position + game->playerZ);
    if (!playerSeg)
        return;
    
    // Only engage if the AI is in the same segment as the player.
    Segment* aiSeg = findSegment(game, driver->z);
    if (aiSeg != playerSeg)
        return;
    
    // Check longitudinal alignment using the reduced trigger distance.
    double playerZ = game->position + game->playerZ;
    double deltaZ = driver->z - playerZ;
    if (fabs(deltaZ) > AI_RAM_DISTANCE_THRESHOLD)
        return;
    
    // If not already ramming and no cooldown remains, initiate a ramming attack.
    if (!driver->isRamming && driver->ramCooldown <= 0.0) {
        driver->isRamming = true;
        driver->ramTime = 0.0;
    }
    
    // If currently ramming, override the target lane offset aggressively.
    if (driver->isRamming) {
        double overshoot = AI_RAM_AGGRESSION_FACTOR;
        if (driver->offset < game->playerX)
            driver->targetLaneOffset = game->playerX + overshoot;
        else
            driver->targetLaneOffset = game->playerX - overshoot;
    }
}

void initAIController(AIController* ctrl, struct Game* game, int numDrivers)
{
    ctrl->game = game;
    ctrl->driverCount = numDrivers;

    // Catchup and behavior parameters
    ctrl->catchupThreshold = 1.0;
    ctrl->cornerSpeedFactor = 0.95;
    ctrl->maxAcceleration   = game->accel-1500;
    ctrl->maxBraking        = fabs(game->breaking);
    ctrl->steeringSensitivity = 0.80;
    ctrl->avoidanceStrength   = 0.02;

    // Allocate the AI drivers
    ctrl->drivers = (AIDriver*)calloc(numDrivers, sizeof(AIDriver));
    for (int i = 0; i < numDrivers; i++) {
        // Position drivers ahead of the player
        ctrl->drivers[i].z = game->position + ((double)(i + 1) * SEGMENT_LENGTH * 15);

        // Initialize lateral offset using lane spacing
        double laneSpacing = 2.0 / (numDrivers + 1);
        ctrl->drivers[i].targetLaneOffset = -1.0 + laneSpacing * (i + 1);
        ctrl->drivers[i].offset = ctrl->drivers[i].targetLaneOffset;
        
        // Speed initialization
        ctrl->drivers[i].speed = 0;
        ctrl->drivers[i].targetSpeed = game->maxSpeed;

        // Lap timing and race progress
        ctrl->drivers[i].lapsCompleted = 0;
        ctrl->drivers[i].lapTime = 0.0;
        ctrl->drivers[i].bestLapTime = 1e9;
        ctrl->drivers[i].finished = false;
        ctrl->drivers[i].totalRaceTime = 0.0;
        ctrl->drivers[i].variant = (i % 3) + 1;

        // Collision state
        ctrl->drivers[i].inCollision = false;

        // Collision animation initialization
        ctrl->drivers[i].collisionAnim.frames = (const Sprite**)malloc(5 * sizeof(Sprite*));
        ctrl->drivers[i].collisionAnim.frameDurations = (double*)malloc(5 * sizeof(double));
        ctrl->drivers[i].collisionAnim.frameCount = 5;
        ctrl->drivers[i].collisionAnim.currentFrame = 0;
        ctrl->drivers[i].collisionAnim.currentTime = 0.0;
        ctrl->drivers[i].collisionAnim.active = false;
        ctrl->drivers[i].collisionAnim.finished = false;
        ctrl->drivers[i].collisionAnim.loop = false;

        // Assign collision animation frames and durations
        ctrl->drivers[i].collisionAnim.frames[0] = &SPRITE_AI_COLLISION_FRAME1;
        ctrl->drivers[i].collisionAnim.frames[1] = &SPRITE_AI_COLLISION_FRAME2;
        ctrl->drivers[i].collisionAnim.frames[2] = &SPRITE_AI_COLLISION_FRAME3;
        ctrl->drivers[i].collisionAnim.frames[3] = &SPRITE_AI_COLLISION_FRAME4;
        ctrl->drivers[i].collisionAnim.frames[4] = &SPRITE_AI_COLLISION_FRAME5;
        for (int j = 0; j < 5; j++) {
            ctrl->drivers[i].collisionAnim.frameDurations[j] = 0.1;
        }

        // Initialize smooth push interpolation fields
        ctrl->drivers[i].pushStartOffset = 0.0;
        ctrl->drivers[i].pushTargetOffset = 0.0;
        ctrl->drivers[i].pushElapsed = 0.0;
        ctrl->drivers[i].pushDuration = 0.5; // 500ms smooth push
        ctrl->drivers[i].isBeingPushed = false;
    }
}

void updateAIController(AIController* ctrl, double dt)
{
    struct Game* game = ctrl->game;
    double trackLength = game->road.trackLength;
    double playerPos   = game->position;

    for (int i = 0; i < ctrl->driverCount; i++) {
        AIDriver* driver = &ctrl->drivers[i];

        // Compute distance between player and AI
        double dist = playerPos - driver->z;

        // Wrapping logic using lap counts
        if (game->playerLapsCompleted > driver->lapsCompleted) {
            // Player has lapped this AI driver => apply wrapping if needed
            if (dist < 0.0) dist += trackLength;
        } else if (game->playerLapsCompleted < driver->lapsCompleted) {
            // AI has lapped the player => apply reverse wrapping if needed
            if (dist > 0.0) dist -= trackLength;
        }

        // Determine catchup speed if AI is too far behind
        if (dist > ctrl->catchupThreshold * trackLength)
            driver->targetSpeed = game->maxSpeed * 1.75;
        else
            driver->targetSpeed = game->maxSpeed;

        // Update the individual AI driver
        updateAIDriver(driver, dt, game, ctrl);
    }
}

void updateAIDriver(AIDriver* driver, double dt, struct Game* game, AIController* ctrl)
{
    double oldZ = driver->z;
    driver->z = increase(driver->z, driver->speed * dt, game->road.trackLength);

    // Only update lap and race time if the AI hasn't finished yet
    if (!driver->finished) {
        driver->lapTime += dt;
        driver->totalRaceTime += dt;
    }

    // Check for lap wrap-around (crossing the finish line)
    if (!driver->finished && driver->z < oldZ) {  // AI crosses the finish line
        driver->lapsCompleted++;  // Increment lap count

        if (driver->lapsCompleted >= game->totalLaps) {  // Check if AI finished the total laps
            driver->finished = true;
            SDL_Log("AI Driver finished the race with laps=%d at time %.2f", driver->lapsCompleted, driver->totalRaceTime);
        }

        // Update best lap time
        if (driver->lapTime < driver->bestLapTime)
            driver->bestLapTime = driver->lapTime;
        
        driver->lapTime = 0.0;  // Reset lap time for the next lap
    }

    // Handle AI driver going off-road
    if ((driver->offset < -1.0) || (driver->offset > 1.0)) {
        // Slow down driver if off-road
        if (driver->speed > game->offRoadLimit)
            driver->speed = accelerate(driver->speed, game->offRoadDecel, dt);
        // Check collision with roadside billboard sprites
        checkAIDriverBillboardCollisions(game, driver);
    }

    // --- Process smooth push interpolation if active
    if (driver->isBeingPushed) {
        driver->pushElapsed += dt;
        float progress = (float)(driver->pushElapsed / driver->pushDuration);
        progress = limit(progress, 0.0f, 1.0f);
        float easedProgress = easeInOutCubic(progress);
        driver->offset = interpolate(driver->pushStartOffset, driver->pushTargetOffset, easedProgress);

        double speedTransferMult = 0.2;
        double speedDifference = game->speed - driver->speed;
        double speedTransferRatio = speedTransferMult * (1.0 - cos(M_PI * driver->pushElapsed / driver->pushDuration));
        driver->speed += speedDifference * speedTransferRatio;
        game->speed -= speedDifference * speedTransferRatio;

        if (progress >= 1.0f) {
            driver->isBeingPushed = false;
            // test target lane offset reset here later
        }
    }
    else {
        // --- Attempt aggressive collision steering toward the player.
        // This will temporarily override the target lane offset to force a collision.
        aiAggressiveSteer(driver, dt, game, ctrl);

        // --- Regular steering toward the target lane offset.
        double offsetError = driver->targetLaneOffset - driver->offset;
        double steerAdjustment = offsetError * ctrl->steeringSensitivity * dt;
        driver->offset += steerAdjustment;
    }

    // --- Manage ramming behavior ---
    if (driver->isRamming) {
        driver->ramTime += dt;
        if (driver->ramTime >= AI_RAM_PERSISTENCE_TIME) {
            // End ramming and set cooldown (with randomness)
            driver->isRamming = false;
            driver->ramCooldown = AI_RAM_ATTACK_COOLDOWN + ((rand() % 100) / 100.0);
        }
    } else {
        // Decrement cooldown if active.
        if (driver->ramCooldown > 0.0)
            driver->ramCooldown -= dt;
    }

    // --- Adjust speed based on upcoming road curvature
    Segment* seg = findSegment(game, driver->z);
    if (!seg)
        return;
    double upcomingCurve = 0.0;
    const int lookAhead = 4;
    for (int i = 1; i <= lookAhead; i++) {
        Segment* fSeg = &game->road.segments[(seg->index + i) % game->road.segmentCount];
        upcomingCurve += fabs(fSeg->curve);
    }
    upcomingCurve /= (double)lookAhead;
    double desiredSpeed = driver->targetSpeed;
    if (upcomingCurve > 2.5)
        desiredSpeed *= ctrl->cornerSpeedFactor;
    if (driver->speed < desiredSpeed)
        driver->speed = accelerate(driver->speed, ctrl->maxAcceleration, dt);
    else {
        double brakeVal = ctrl->maxBraking * 0.5;
        driver->speed = accelerate(driver->speed, -brakeVal, dt);
    }
    driver->speed = limit(driver->speed, 0.0, driver->targetSpeed * 1.2);

    driver->percent = percentRemaining(driver->z, SEGMENT_LENGTH);

    // Reset collision state after processing
    if (driver->inCollision) {
        driver->lastCollisionType = COLLISION_NONE;
        driver->inCollision = false;
    }
    // --- Unstuck Mechanism: if AI is stuck (speed is very low) and its offset deviates from its target,
    // initiate a push toward the targetLaneOffset.
    const double AI_UNSTUCK_SPEED_THRESHOLD = game->maxSpeed * 0.1; // 10% of max speed
    const double AI_UNSTUCK_OFFSET_TOLERANCE = 0.05;
    if (driver->speed < AI_UNSTUCK_SPEED_THRESHOLD &&
        fabs(driver->offset - driver->targetLaneOffset) > AI_UNSTUCK_OFFSET_TOLERANCE &&
        !driver->isBeingPushed) {
        driver->isBeingPushed = true;
        driver->pushStartOffset = driver->offset;
        driver->pushTargetOffset = driver->targetLaneOffset;
        driver->pushElapsed = 0.0;
        driver->pushDuration = 0.3; // Quicker push to get unstuck
        SDL_Log("AI unstuck: initiating push from offset %.3f to target %.3f", driver->offset, driver->targetLaneOffset);
    }
}