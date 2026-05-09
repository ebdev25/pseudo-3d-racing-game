// collision.c
#include "collision.h"
#include "util.h"
#include "constants.h"
#include <SDL2/SDL.h>

#define LONGITUDINAL_COLLISION_THRESHOLD (SEGMENT_LENGTH * 4.75)

// Computes AI scale based on perspective
static double computeAIScale(Game* game, Segment* aiSegment, double aiZ, double aiOffset) {
    (void)aiOffset; // Unused parameter
    if (!aiSegment) return 0.0;

    double trackLength = game->road.trackLength;
    
    // Compute AI's position relative to player's camera
    double aiCameraZ = aiZ - game->position;
    if (aiCameraZ < -trackLength / 2) aiCameraZ += trackLength;
    else if (aiCameraZ > trackLength / 2) aiCameraZ -= trackLength;

    // If AI behind camera, return 0 scale
    if (aiCameraZ <= 0) return 0.0;

    // Perspective projection scale calculation
    double aiScale = game->cameraDepth / aiCameraZ;
    aiScale *= 1000.25;  // Scale up by a lot to give useable values

    return aiScale;
}

void checkAIDriverBillboardCollisions(Game* game, AIDriver* driver) {
    // Get the segment where the AI driver is located
    Segment* aiSegment = findSegment(game, driver->z);
    if (!aiSegment)
        return;
    
    // Determine AI driver's sprite width
    double aiW = (driver->sprite) ? driver->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;
    
    // Iterate over all billboard sprites in segment
    for (int n = 0; n < aiSegment->spriteCount; n++) {
        SpriteInstance* sprite = &aiSegment->sprites[n];

        // Determine the width of the collision box, using the custom width if specified
        double spriteW = sprite->source->useCustomCollisionBox ?
                         sprite->source->collisionWidth * SPRITE_SCALE :
                         sprite->source->w * SPRITE_SCALE;

        // Determine the offset of the collision box from the sprite's main anchor point
        double spriteOffsetX = sprite->source->useCustomCollisionBox ?
                               sprite->source->collisionOffsetX * SPRITE_SCALE :
                               0.0;
        
        // BUG FIX: Calculate the CENTER of the sprite's collision box for the overlap check.
        // The overlap() function requires the center point, not the left edge.
        double spriteCollisionBoxCenter = sprite->offset + spriteOffsetX;

        // Check for overlap between AI's lateral position and sprite's position
        if (overlap(driver->offset, aiW, spriteCollisionBoxCenter, spriteW, 1.0)) {
            // On collision, reduce AI driver's speed and reset its position within segment
            driver->speed = game->maxSpeed / 5.0;
            driver->z = aiSegment->p1.world.z;  // Reset to the start of segment
            SDL_Log("Collision with roadside billboard detected for AI driver. Speed reduced and position reset.\n");
            break;
        }
    }
}

void checkPlayerAIOpponentCollisions(Game* game) {
    Segment* playerSegment = findSegment(game, game->position + game->playerZ);
    if (!playerSegment) return;

    const double playerW = SPRITE_PLAYER_STRAIGHT.w * SPRITE_SCALE;
    const double trackLength = game->road.trackLength;
    const double playerZ = game->position + game->playerZ;

    for (int i = 0; i < game->aiController.driverCount; i++) {
        AIDriver* driver = &game->aiController.drivers[i];
        Segment* aiSegment = findSegment(game, driver->z);
        if (!aiSegment) continue;

        const double aiZ = driver->z;
        const double aiW = driver->sprite ? driver->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;

        // Calculate longitudinal distance with track wrapping
        double deltaZ = aiZ - playerZ;
        if (deltaZ < -trackLength / 2) deltaZ += trackLength;
        else if (deltaZ > trackLength / 2) deltaZ -= trackLength;
        double absDeltaZ = fabs(deltaZ);

        // Calculate lateral boundaries
        double playerLeft  = game->playerX - playerW / 2;
        double playerRight = game->playerX + playerW / 2;
        double aiLeft      = driver->offset - aiW / 2;
        double aiRight     = driver->offset + aiW / 2;
        bool lateralOverlap = (playerRight > aiLeft && playerLeft < aiRight);

        // REAR COLLISION CHECK
        bool isRearCollision = (deltaZ > 0) && 
                               (absDeltaZ < LONGITUDINAL_COLLISION_THRESHOLD) && 
                               lateralOverlap;

        // Compute AI sprite's current draw scale
        double aiScale = computeAIScale(game, aiSegment, aiZ, driver->offset);

        // Define a threshold for when side collisions should activate
        const double SIDE_COLLISION_SCALE_THRESHOLD = 0.490;

        // Check if AI's scale is above threshold (if close enough for side collisions)
        bool sideCollisionsEnabled = (aiScale > SIDE_COLLISION_SCALE_THRESHOLD);

        if (isRearCollision) {
            if (sideCollisionsEnabled) {
                double targetX = (game->playerX > driver->offset) ? ((aiRight + (playerW / 2)) + 0.05) : ((aiLeft - (playerW / 2)) - 0.05);
                game->playerX = easeInOut(game->playerX, targetX, 0.25);
            }

            if (!driver->isBeingPushed) { // Only apply push when entering collision
                double pushDir = (driver->offset > game->playerX) ? 1.0 : -1.0;
                driver->isBeingPushed = true;
                driver->pushStartOffset = driver->offset;
                driver->pushTargetOffset = driver->offset + (pushDir * 0.5); // Push 0.5 lane sideways
                driver->pushElapsed = 0.0;
            }
            driver->inCollision = true;
            continue;
        } else if (deltaZ < 0 && absDeltaZ < LONGITUDINAL_COLLISION_THRESHOLD && lateralOverlap) {
            // Player is behind the AI and collision occurs
            game->speed *= 0.99; // Reduce player speed slightly

            // Trigger the push state on the AI
            if (!driver->isBeingPushed) { // Only trigger if not already being pushed
                // Determine push direction for AI (away from player)
                double pushDir = (game->playerX > driver->offset) ? -1.0 : 1.0;
                driver->isBeingPushed = true;
                driver->pushStartOffset = driver->offset;
                // Push the AI sideways by a defined amount
                driver->pushTargetOffset = driver->offset + (pushDir * 0.5);
                driver->pushElapsed = 0.0;
            }
            driver->inCollision = true; // Mark AI as being in collision

            continue; // Skip setting inCollision to false
        }
        // NO COLLISION DETECTED
        driver->inCollision = false;
        driver->lastCollisionType = COLLISION_NONE;
    }

    // Clamp values
    game->speed = limit(game->speed, 0, game->maxSpeed);
    for (int i = 0; i < game->aiController.driverCount; i++) {
        game->aiController.drivers[i].speed = limit(game->aiController.drivers[i].speed, 0, game->maxSpeed * 1.2);
    }
    game->playerX = limit(game->playerX, -3.0, 3.0);
}

void checkAIDriverCollisions(Game* game) {
    if (game->totalRaceTime < 3.00 || game->speed < 1.0) {
        return; // Skip AI collision checks entirely during this initial period
    }
    // Add Checks for Game State Validity
    // Ensure game pointer is valid
    if (!game) {
        SDL_Log("Error: game pointer is NULL in checkAIDriverCollisions.");
        return;
    }
    // Ensure AI controller and drivers array are valid, and count is non-negative
    if (!game->aiController.drivers || game->aiController.driverCount <= 0) {
        // No drivers to check or invalid state, safely return
        return;
    }
    // Use the validated count
    int count = game->aiController.driverCount;
    // End Checks


    double trackLength = game->road.trackLength;

    for (int i = 0; i < count; i++) { // Loop up to validated count
        // Bounds check implicitly handled by loop condition (i < count)
        AIDriver* driverA = &game->aiController.drivers[i];

        // Ensure driverA pointer itself isn't corrupted/unexpectedly NULL
        if (!driverA) {
             SDL_Log("Warning: driverA is NULL in checkAIDriverCollisions loop (i=%d)", i);
             continue; // Skip iteration
        }

        Segment* segA = findSegment(game, driverA->z);
        if (!segA) continue; // Keep existing segment check

        // Check sprite pointer before dereferencing width
        double aiAWidth = (driverA->sprite) ? driverA->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;
        double aLeft = driverA->offset - aiAWidth / 2;
        double aRight = driverA->offset + aiAWidth / 2;

        for (int j = i + 1; j < count; j++) { // Loop up to validated 'count'
            // Bounds check implicitly handled by loop condition (j < count)
            AIDriver* driverB = &game->aiController.drivers[j];

             // Ensure driverB pointer itself isn't corrupted/unexpectedly NULL
             if (!driverB) {
                  SDL_Log("Warning: driverB is NULL in checkAIDriverCollisions loop (j=%d)", j);
                  continue; // Skip iteration
             }

            Segment* segB = findSegment(game, driverB->z);
            if (!segB) continue; // Keep existing segment check

            // Check sprite pointer before dereferencing width
            double aiBWidth = (driverB->sprite) ? driverB->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;
            double bLeft = driverB->offset - aiBWidth / 2;
            double bRight = driverB->offset + aiBWidth / 2;

            // Calculate longitudinal distance with track wrapping
            double deltaZ = driverA->z - driverB->z;
            if (deltaZ < -trackLength / 2) deltaZ += trackLength;
            else if (deltaZ > trackLength / 2) deltaZ -= trackLength;
            double absDeltaZ = fabs(deltaZ);

            // Only consider collisions if the drivers are close enough in z
            if (absDeltaZ < LONGITUDINAL_COLLISION_THRESHOLD) {
                bool lateralOverlap = (aRight > bLeft && aLeft < bRight);
                if (lateralOverlap) {
                    // Define the push amount
                    double pushAmount = 0.5;

                    // For driverA: set push if not already in a push state
                    if (!driverA->isBeingPushed) {
                        driverA->isBeingPushed = true;
                        driverA->pushStartOffset = driverA->offset;
                        if (driverA->offset < driverB->offset)
                            driverA->pushTargetOffset = driverA->offset - pushAmount;
                        else
                            driverA->pushTargetOffset = driverA->offset + pushAmount;
                        driverA->pushElapsed = 0.0;
                    }

                    // For driverB: similarly set push
                    if (!driverB->isBeingPushed) {
                        driverB->isBeingPushed = true;
                        driverB->pushStartOffset = driverB->offset;
                        if (driverB->offset < driverA->offset)
                            driverB->pushTargetOffset = driverB->offset - pushAmount;
                        else
                            driverB->pushTargetOffset = driverB->offset + pushAmount;
                        driverB->pushElapsed = 0.0;
                    }

                    // Mark both as in collision
                    driverA->inCollision = true;
                    driverB->inCollision = true;
                }
            }
        }
    }
}