#include "ai.h"
#include "game.h"
#include "util.h"
#include "constants.h"
#include "road.h"
#include "collision.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdalign.h>
#include <string.h>

#define AI_RAM_AGGRESSION_FACTOR 0.5  // lateral aggression
#define AI_RAM_DISTANCE_THRESHOLD (SEGMENT_LENGTH * 0.75)  // ramming distance
#define AI_RAM_PERSISTENCE_TIME 1.2  // AI will continue attacking for 1.5 seconds
#define AI_RAM_ATTACK_COOLDOWN 2.0  // AI waits this long before attacking again

static bool trySelectRammingBehavior(AIDriver* driver, struct Game* game, AIController* ctrl) {
    if (driver->aggressionFactor <= 0.0 || driver->ramCooldown > 0.0)
        return false;

    bool targetFound = false;
    double targetX = 0.0;

    // Check for player target
    // Find player's current segment
    Segment* playerSeg = findSegment(game, game->position + game->playerZ);
    // Find AI driver's current segment
    Segment* aiSeg = findSegment(game, driver->z);
    // Proceed if player segment valid and same as AI segment
    if (playerSeg && aiSeg == playerSeg) {
        // Get player Z position
        double playerZ = game->position + game->playerZ;
        // Calculate longitudinal distance AI to player
        double deltaZ = driver->z - playerZ;
        // Check if player within longitudinal ramming distance
        if (fabs(deltaZ) <= AI_RAM_DISTANCE_THRESHOLD) {
            // Player is valid ram target
            targetFound = true;
            // Store player lateral position for ramming aim
            targetX = game->playerX;
        }
    }

    // Check for AI target if player not found
        // Only search for AI target if player target was not found
        if (!targetFound) {
            // Iterate through all AI drivers
            for (int i = 0; i < ctrl->driverCount; i++) {
                // Get pointer to potential target AI
                AIDriver* other = &ctrl->drivers[i];
                // Skip self-targeting
                if (other == driver) continue;

                // Find segment for current AI driver
                Segment* segDriver = findSegment(game, driver->z);
                // Find segment for other AI driver
                Segment* segOther = findSegment(game, other->z);
                // Skip if segments invalid or different
                if (!segDriver || !segOther || segDriver != segOther) continue;

                // Get total track length for wrapping calculation
                double trackLength = game->road.trackLength;
                // Calculate longitudinal distance current AI to other AI
                double deltaZ = driver->z - other->z;
                // Handle track wrap-around for distance calculation
                if (deltaZ < -trackLength/2) deltaZ += trackLength;
                else if (deltaZ > trackLength/2) deltaZ -= trackLength;

                // Check if other AI within longitudinal ramming distance
                if (fabs(deltaZ) <= AI_RAM_DISTANCE_THRESHOLD) {
                    // Other AI is valid ram target
                    targetFound = true;
                    // Store other AI lateral position for ramming aim
                    targetX = other->offset;
                    // Found target stop searching
                    break;
                }
            }
        }

    // Evaluate ramming if a target is valid
        // Proceed only if valid target (player or AI) was found
        if (targetFound) {
            // Calculate probability of ramming based on aggression
            float ramProbability = 0.1f + driver->aggressionFactor * 0.8f;
            // Roll random number for probability check
            float roll = (float)(rand() % 101) / 100.0f;

            // Check if random roll succeeds against probability
            if (roll < ramProbability) {
                // Set AI behavior to ramming
                driver->currentBehavior = AI_BEHAVIOR_RAMMING;
                // Set ramming state flag
                driver->isRamming = true;
                // Reset ramming persistence timer
                driver->ramTime = 0.0;

                // Get lateral overshoot amount for aiming
                double overshoot = AI_RAM_AGGRESSION_FACTOR;
                // Set target offset slightly past target X position
                driver->targetLaneOffset = (driver->offset < targetX)
                    ? targetX + overshoot
                    : targetX - overshoot;

                // Set increased steering speed for ramming
                driver->driftSpeed = ctrl->steeringSensitivity * 1.75;
                // Indicate ramming behavior was selected
                return true;
            }
        }

    return false;
}

static void selectNewBehavior(AIDriver* driver, struct Game* game, AIController* ctrl) {
    driver->behaviorElapsed = 0.0;
    if (trySelectRammingBehavior(driver, game, ctrl))
        return;  // Ramming behavior selected
    driver->isRamming = false;

    int roll = rand() % 100;

    if (roll < 55) {
        // WOBBLE
        driver->currentBehavior = AI_BEHAVIOR_WOBBLE;
        double range = 0.1 + ((rand() % 100) / 100.0) * 0.1;
        int direction = (rand() % 2 == 0) ? 1 : -1;
        driver->targetLaneOffset = limit(driver->offset + range * direction, -0.75, 0.75);
        driver->driftSpeed = ((rand() % 100) / 100.0) * 0.25 + 0.1;
    }
    else if (roll < 85) {
        // HOLD_LINE
        driver->currentBehavior = AI_BEHAVIOR_HOLD_LINE;
        driver->targetLaneOffset = driver->offset;
        driver->driftSpeed = 0;
    }
    else if (roll < 95) {
        // LANE_SWITCH
        driver->currentBehavior = AI_BEHAVIOR_LANE_SWITCH;
        double leftTarget = driver->offset - 0.6;
        double rightTarget = driver->offset + 0.6;

        int canGoLeft = leftTarget >= -0.75;
        int canGoRight = rightTarget <= 0.75;
        int direction = 0;

        if (canGoLeft && canGoRight) {
            direction = (rand() % 2 == 0) ? -1 : 1;
        } else if (canGoLeft) {
            direction = -1;
        } else if (canGoRight) {
            direction = 1;
        } else {
            driver->currentBehavior = AI_BEHAVIOR_HOLD_LINE;
            driver->targetLaneOffset = driver->offset;
            driver->driftSpeed = 0;
            return;
        }

        driver->targetLaneOffset = driver->offset + direction * 0.6;
        driver->driftSpeed = 0.4;
    }
    else if (roll < 98) {
        // SPEEDUP
        driver->currentBehavior = AI_BEHAVIOR_SPEEDUP;
        driver->speedupPhase = 0;
        driver->speedupTime = 0.0;

        float boostFactor = 1.05f; // 5% boost
        driver->speedupTargetSpeed = driver->targetSpeed * boostFactor;
        driver->speedupDuration = 0.8f + ((rand() % 30) / 100.0f); // ~0.8–1.1s
    }
    else {
        // SLOWDOWN
        driver->currentBehavior = AI_BEHAVIOR_SLOWDOWN;
        driver->slowdownPhase = 0;
        driver->slowdownTime = 0.0;

        float speedFactor = 0.75f + ((rand() % 26) / 100.0f); // Between 0.75x and 1.0x of maxSpeed
        driver->slowdownTargetSpeed = driver->targetSpeed * speedFactor;
    }
}

// Applies lateral steering based on current behavior's target offset and drift speed
static void steerWithBehavior(AIDriver* driver, double dt) {
    // Calculate difference between the target lateral offset + current offset
    double delta = driver->targetLaneOffset - driver->offset;
    // If AI very close to target offset, snap directly to it, avoids small jitters
    if (fabs(delta) < 0.01) {
        driver->offset = driver->targetLaneOffset;
    } else {
        // Determine direction to move towards target offset (1 right, -1 left)
        double direction = (delta > 0) ? 1 : -1;
        // Adjust current offset based on direction, behavior specific drift speed, delta time
        driver->offset += direction * driver->driftSpeed * dt;
    }
    // Limit AI's lateral pos unless recovering/being pushed by collision
    // prevents extreme offsets during normal behavior steering
    if (!driver->recovering && !driver->isBeingPushed) {
        // Clamp offset within range
        driver->offset = limit(driver->offset, -8.0, 8.0);
    }
}

// Handles AI recovery state, steering back to target + slowing down
static int handleRecovery(AIDriver* driver, double dt, struct Game* game) {
    // Exit if driver not recovering
    if (!driver->recovering) return 0;
    // Steer driver slowly back towards target lane offset
    if (driver->offset > driver->targetLaneOffset)
        // Steer left if right of target
        driver->offset -= 0.05 * dt;
    else
        // Steer right if left of target
        driver->offset += 0.05 * dt;
    // Reduce speed significantly during recovery
    driver->speed = accelerate(driver->speed, -game->maxSpeed / 3, dt);
    // Ensure minimum speed limit during recovery
    if (driver->speed < game->maxSpeed * 0.1)
        driver->speed = game->maxSpeed * 0.1;
    // Check if driver close enough to target offset to end recovery
    if (fabs(driver->offset - driver->targetLaneOffset) <= 0.05) {
        // End recovery state
        driver->recovering = 0;
        // Update target offset to current to prevent immediate re-steer
        driver->targetLaneOffset = driver->offset;
        // Reset behavior state after recovery
        driver->currentBehavior = AI_BEHAVIOR_NONE;
    }
    // Return 1 indicating recovery was processed this frame
    return 1;
}

// Updates AI steering + speed based on current selected behavior
static void updateBehaviorSteering(AIDriver* driver, double dt, struct Game* game, AIController* ctrl) {
    // Select new behavior if none currently active
    if (driver->currentBehavior == AI_BEHAVIOR_NONE) {
        selectNewBehavior(driver, game, ctrl);
    }
    // Track time elapsed in current behavior
    driver->behaviorElapsed += dt;
    // Reset behavior if it runs too long (timeout safety)
    if (driver->behaviorElapsed >= 5.0) {
        SDL_Log("Behavior timeout for AI driver — resetting to NONE");
        // Reset behavior to none
        driver->currentBehavior = AI_BEHAVIOR_NONE;
        // Reset target lane to default
        driver->targetLaneOffset = driver->defaultLaneOffset;
        // Reset timer
        driver->behaviorElapsed = 0.0;
    }
    // Handle logic based on current behavior type
    switch (driver->currentBehavior) {
        case AI_BEHAVIOR_RAMMING:
            // Apply ramming steering logic
            steerWithBehavior(driver, dt);

            // Manage ramming state timing
            if (driver->isRamming) {
                // Increment ram timer
                driver->ramTime += dt;
                // Check if ramming duration exceeded
                if (driver->ramTime >= AI_RAM_PERSISTENCE_TIME) {
                    // Stop ramming
                    driver->isRamming = false;
                    // Set cooldown with randomness
                    driver->ramCooldown = AI_RAM_ATTACK_COOLDOWN + ((rand() % 100) / 100.0);
                    // Reset behavior + target lane
                    driver->currentBehavior = AI_BEHAVIOR_NONE;
                    driver->targetLaneOffset = driver->defaultLaneOffset;
                }
            }
            break;

        case AI_BEHAVIOR_WOBBLE:
        case AI_BEHAVIOR_HOLD_LINE:
        case AI_BEHAVIOR_LANE_SWITCH:
            // Apply steering for lateral movement behaviors
            steerWithBehavior(driver, dt);
            // If target offset reached, reset behavior
            if (fabs(driver->offset - driver->targetLaneOffset) <= 0.01) {
                driver->currentBehavior = AI_BEHAVIOR_NONE;
            }
            break;
        case AI_BEHAVIOR_SPEEDUP:
            // Handle speedup state machine
            switch (driver->speedupPhase) {
                case 0: // Phase 0: Accelerate to boosted speed
                    driver->speed = accelerate(driver->speed, ctrl->maxAcceleration, dt);
                    // Check if boosted speed reached
                    if (driver->speed >= driver->speedupTargetSpeed - 2.0) {
                        // Snap to target, advance phase, reset timer
                        driver->speed = driver->speedupTargetSpeed;
                        driver->speedupPhase = 1;
                        driver->speedupTime = 0.0;
                    }
                    break;

                case 1: // Phase 1: Hold boosted speed for duration
                    driver->speedupTime += dt;
                    // Check if duration elapsed
                    if (driver->speedupTime >= driver->speedupDuration) {
                        // Advance phase, reset timer
                        driver->speedupPhase = 2;
                        driver->speedupTime = 0.0;
                    }
                    break;

                case 2: // Phase 2: Decelerate back to normal target speed
                    driver->speed = accelerate(driver->speed, -ctrl->maxBraking, dt);
                    // Check if normal speed reached
                    if (driver->speed <= driver->targetSpeed + 2.0) {
                        // Snap to target, reset behavior
                        driver->speed = driver->targetSpeed;
                        driver->currentBehavior = AI_BEHAVIOR_NONE;
                    }
                    break;
            }
            // Apply steering during speedup
            steerWithBehavior(driver, dt);
            break;
        case AI_BEHAVIOR_SLOWDOWN:
            // Handle slowdown state machine
            switch (driver->slowdownPhase) {
                case 0: { // Phase 0: Decelerate to slower target speed
                    driver->speed = accelerate(driver->speed, -ctrl->maxBraking * 0.5, dt);
                    // Check if slow speed reached
                    if (driver->speed <= driver->slowdownTargetSpeed + 5.0) {
                        // Snap to target, advance phase, reset timer
                        driver->speed = driver->slowdownTargetSpeed;
                        driver->slowdownPhase = 1;
                        driver->slowdownTime = 0.0;
                    }
                    break;
                }

                case 1: { // Phase 1: Hold slow speed for fixed duration
                    driver->slowdownTime += dt;
                    // Check if duration elapsed
                    if (driver->slowdownTime >= 1.5) {
                        // Advance phase, reset timer
                        driver->slowdownPhase = 2;
                        driver->slowdownTime = 0.0;
                    }
                    break;
                }

                case 2: { // Phase 2: Accelerate back to normal target speed
                    driver->speed = accelerate(driver->speed, ctrl->maxAcceleration, dt);
                    // Check if normal speed reached
                    if (driver->speed >= driver->targetSpeed - 5.0) {
                        // Snap to target, reset behavior
                        driver->speed = driver->targetSpeed;
                        driver->currentBehavior = AI_BEHAVIOR_NONE;
                    }
                    break;
                }
            }
            // Apply steering during slowdown
            steerWithBehavior(driver, dt);
            break;
        case AI_BEHAVIOR_NONE:
        default: // Default case if behavior is NONE or unexpected
            // Set target to current offset (stop drifting)
            driver->targetLaneOffset = driver->offset;
            // Apply steering
            steerWithBehavior(driver, dt);
            break;
    }
}

void initAIController(AIController* ctrl, struct Game* game, int numDrivers)
{
    ctrl->game = game;
    ctrl->driverCount = numDrivers;

    // Catchup and behavior parameters
    ctrl->catchupThreshold = 0.1; // how far back ai from player once catchup mechanics start
    ctrl->cornerSpeedFactor = 0.95;
    ctrl->maxAcceleration   = game->accel-200;
    ctrl->maxBraking        = fabs(game->breaking);
    ctrl->steeringSensitivity = 0.80;
    ctrl->avoidanceStrength   = 0.02;

    // Separation variables
    const double base_z_offset = SEGMENT_LENGTH * 15.0; // Z distance for the front row
    const double z_stagger     = SEGMENT_LENGTH * 5.0;  // How far ahead/behind to stagger
    // 3 primary lateral positions
    const double lat_positions[3] = {-0.7, 0.0, 0.7}; // normalized spacing
    const int max_row_width = 3;

    // Allocate the AI drivers
    size_t total_size = sizeof(AIDriver) * numDrivers;
    ctrl->drivers = arena_alloc(&game->levelArena, total_size, alignof(AIDriver));
    if (!ctrl->drivers) {
        SDL_Log("Error: Allocation for AI Drivers into Level Arena failed.");
        ctrl->driverCount = 0;
        return;
    }
    memset(ctrl->drivers, 0, total_size);
    for (int i = 0; i < numDrivers; i++) {
        // Staggered position fields
        double assigned_z;
        double assigned_offset;
        int row = i / max_row_width; // Determines row (0 = front, 1 = second row, etc)
        int pos_in_row = i % max_row_width; // Position within row (0-2)
        if (row == 0) { // First row (max 3 drivers: i=0, 1, 2)
            assigned_z = base_z_offset;
            assigned_offset = lat_positions[pos_in_row];
        } else if (row == 1) { // Second row (drivers i=3, 4)
            assigned_z = base_z_offset - z_stagger; // Place behind first row
            // Re-use lateral pos
            assigned_offset = lat_positions[pos_in_row == 0 ? 0 : 2];
        } else {
             assigned_z = base_z_offset - (z_stagger * row);
             assigned_offset = lat_positions[pos_in_row];
             SDL_Log("Warning: Placing AI driver %d in fallback starting position.", i);
        }

        // Initialize driver z lateral offset using assigned offset
        ctrl->drivers[i].z = assigned_z;
        ctrl->drivers[i].targetLaneOffset = assigned_offset;
        ctrl->drivers[i].offset = assigned_offset;
        ctrl->drivers[i].defaultLaneOffset = assigned_offset;
        // Speed initialization
        ctrl->drivers[i].speed = 0;
        ctrl->drivers[i].targetSpeed = game->maxSpeed;

        // Lap timing and race progress
        ctrl->drivers[i].lapsCompleted = 0;
        ctrl->drivers[i].lapTime = 0.0;
        ctrl->drivers[i].bestLapTime = 1e9;
        ctrl->drivers[i].finished = false;
        ctrl->drivers[i].totalRaceTime = 0.0;
        ctrl->drivers[i].variant = (i % 5) + 1;

        // Collision state
        ctrl->drivers[i].inCollision = false;

        // Initialize smooth push interpolation fields
        ctrl->drivers[i].pushStartOffset = 0.0;
        ctrl->drivers[i].pushTargetOffset = 0.0;
        ctrl->drivers[i].pushElapsed = 0.0;
        ctrl->drivers[i].pushDuration = 0.5; // 500ms smooth push
        ctrl->drivers[i].isBeingPushed = false;

        ctrl->drivers[i].recovering = 0;
        ctrl->drivers[i].currentBehavior = AI_BEHAVIOR_NONE;
        ctrl->drivers[i].driftSpeed = 0;
        ctrl->drivers[i].aggressionFactor = limit(1.0f - ((float)i / (float)numDrivers), 0.0f, 1.0f);
        ctrl->drivers[i].slowdownTime = 0.0f;

        ctrl->drivers[i].slowdownDuration = 0.0f;
        ctrl->drivers[i].slowdownTargetSpeed = 0.0f;
        ctrl->drivers[i].slowdownPhase = 0;

        ctrl->drivers[i].speedupTime = 0.0f;
        ctrl->drivers[i].speedupDuration = 0.0f;
        ctrl->drivers[i].speedupTargetSpeed = 0.0f;
        ctrl->drivers[i].speedupPhase = 0;
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
            // Player has lapped this AI driver => apply wrapping
            if (dist < 0.0) dist += trackLength;
        } else if (game->playerLapsCompleted < driver->lapsCompleted) {
            // AI has lapped the player => apply reverse wrapping
            if (dist > 0.0) dist -= trackLength;
        }

        // Determine catchup speed if AI is too far behind
        if (dist > ctrl->catchupThreshold * trackLength)
            driver->targetSpeed = game->maxSpeed * 3.5; // % faster for catch up
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

    bool canSelectBehavior = false;

    if (game->raceState == RACE_STATE_RUNNING && driver->initialBehaviorDelayTimer > 0.0) {
        driver->initialBehaviorDelayTimer -= dt;
    }

    // Check if the delay timer has expired
    if (driver->initialBehaviorDelayTimer <= 0.0) {
        canSelectBehavior = true;
    }

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

    // Process smooth push interpolation if active
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
        //game->speed -= speedDifference * speedTransferRatio;

        if (progress >= 1.0f) {
            driver->isBeingPushed = false;
            driver->targetLaneOffset = driver->defaultLaneOffset; // Reset target after push
            driver->currentBehavior = AI_BEHAVIOR_NONE; // Ensure behavior resets after push
        }
    }
    else { // NOT Being Pushed
        // Apply or select behavior
        if (canSelectBehavior) {
            updateBehaviorSteering(driver, dt, game, ctrl);

            // Handle Recovery
            if (handleRecovery(driver, dt, game)) {
            }
            // apply general steering logic if no other behavior active
            else if (driver->currentBehavior == AI_BEHAVIOR_NONE ||
                    driver->currentBehavior == AI_BEHAVIOR_HOLD_LINE ||
                    driver->currentBehavior == AI_BEHAVIOR_SPEEDUP ||
                    driver->currentBehavior == AI_BEHAVIOR_SLOWDOWN)
            {
                // Regular steering toward the target lane offset
                // If HOLD_LINE active, targetLaneOffset should already be == offset
                // If NONE, steers towards whatever targetLaneOffset is currently
                double offsetError = driver->targetLaneOffset - driver->offset;
                double steerAdjustment = offsetError * ctrl->steeringSensitivity * dt;
                driver->offset += steerAdjustment;
            }
        } else {
            double offsetError = driver->defaultLaneOffset - driver->offset;
            double steerAdjustment = offsetError * ctrl->steeringSensitivity * dt * 0.5;
            driver->offset += steerAdjustment;
            driver->currentBehavior = AI_BEHAVIOR_NONE;
        }
    }

    // Adjust speed based on upcoming road curvature
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
    // Unstuck Mechanism: if AI stuck (speed very low)/its offset deviates from target,
    // initiate push to targetLaneOffset
    const double AI_UNSTUCK_SPEED_THRESHOLD = game->maxSpeed * 0.1; // 10% of max speed
    const double AI_UNSTUCK_OFFSET_TOLERANCE = 0.05;
    if (game->totalRaceTime > 1.0 && driver->speed < AI_UNSTUCK_SPEED_THRESHOLD &&
        fabs(driver->offset - driver->targetLaneOffset) > AI_UNSTUCK_OFFSET_TOLERANCE &&
        !driver->isBeingPushed) {
        driver->isBeingPushed = true;
        driver->pushStartOffset = driver->offset;
        driver->pushTargetOffset = driver->targetLaneOffset;
        driver->pushElapsed = 0.0;
        driver->pushDuration = 0.3;
        SDL_Log("AI unstuck: initiating push from offset %.3f to target %.3f", driver->offset, driver->targetLaneOffset);
    }
}