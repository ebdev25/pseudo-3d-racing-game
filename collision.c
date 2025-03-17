// collision.c
#include "collision.h"
#include "util.h"
#include "constants.h"
#include <SDL2/SDL.h>

#define LONGITUDINAL_COLLISION_THRESHOLD (SEGMENT_LENGTH * 4.75)

// Computes AI scale based on perspective
static double computeAIScale(Game* game, Segment* aiSegment, double aiZ, double aiOffset) {
    if (!aiSegment) return 0.0;

    double trackLength = game->road.trackLength;
    
    // Compute AI's position relative to the player's camera
    double aiCameraZ = aiZ - game->position;
    if (aiCameraZ < -trackLength / 2) aiCameraZ += trackLength;
    else if (aiCameraZ > trackLength / 2) aiCameraZ -= trackLength;

    // If AI is behind the camera, return 0 scale
    if (aiCameraZ <= 0) return 0.0;

    // Interpolate world Y position for AI
    double aiWorldY = interpolate(aiSegment->p1.world.y, aiSegment->p2.world.y, percentRemaining(aiZ, SEGMENT_LENGTH));
    double aiCameraY = aiWorldY + game->cameraDepth;

    // Perspective projection scale calculation
    double aiScale = game->cameraDepth / aiCameraZ;
    aiScale *= 1000.25;  // Scale up by a lot to give useable values

    return aiScale;
}

void checkAIDriverBillboardCollisions(Game* game, AIDriver* driver) {
    // Get the segment where the AI driver is located.
    Segment* aiSegment = findSegment(game, driver->z);
    if (!aiSegment)
        return;
    
    // Determine the AI driver's sprite width.
    double aiW = (driver->sprite) ? driver->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;
    
    // Iterate over all billboard sprites in the segment.
    for (int n = 0; n < aiSegment->spriteCount; n++) {
        SpriteInstance* sprite = &aiSegment->sprites[n];
        double spriteW = sprite->source->w * SPRITE_SCALE;
        // Calculate the sprite's center offset.
        double spriteCenterOffset = sprite->offset + ((sprite->offset > 0.0) ? (spriteW / 2.0) : (-spriteW / 2.0));
        
        // Check for overlap between the AI's lateral position and the sprite's position.
        if (overlap(driver->offset, aiW, spriteCenterOffset, spriteW, 1.0)) {
            // On collision, reduce the AI driver's speed and reset its position within the segment.
            driver->speed = game->maxSpeed / 5.0;
            driver->z = aiSegment->p1.world.z;  // Reset to the start of the segment.
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

        // **REAR COLLISION CHECK**
        bool isRearCollision = (deltaZ > 0) && 
                               (absDeltaZ < LONGITUDINAL_COLLISION_THRESHOLD) && 
                               lateralOverlap;

        // Compute AI sprite's current draw scale
        double aiScale = computeAIScale(game, aiSegment, aiZ, driver->offset);

        // Define a threshold for when side collisions should activate
        const double SIDE_COLLISION_SCALE_THRESHOLD = 0.490;  // Adjust

        // Check if AI's scale is above the threshold (i.e., if close enough for side collisions)
        bool sideCollisionsEnabled = (aiScale > SIDE_COLLISION_SCALE_THRESHOLD);

        if (isRearCollision) {
            if (sideCollisionsEnabled) { // Need this block to give player block-back when player is hitting an ai car
                double targetX = (game->playerX > driver->offset) ? ((aiRight + (playerW / 2)) + 0.05) : ((aiLeft - (playerW / 2)) - 0.05);
                //game->playerX = (game->playerX * 0.75) + (targetX * 0.25);  // Gradual movement
                //game->playerX = easeOut(game->playerX, targetX, 0.25);
                game->playerX = easeInOut(game->playerX, targetX, 0.25);
            }

            if (!driver->isBeingPushed) { // Only apply push when entering collision
                double pushDir = (driver->offset > game->playerX) ? 1.0 : -1.0;
                driver->isBeingPushed = true;
                driver->pushStartOffset = driver->offset;
                driver->pushTargetOffset = driver->offset + (pushDir * 0.5); // Push 0.5 lane sideways
                driver->pushElapsed = 0.0;
            }
            
            SDL_Log("[COL] Collision locked | Speeds (P/A): %.1f/%.1f", game->speed, driver->speed);
            driver->inCollision = true;
            continue;
        }
        // **NO COLLISION DETECTED**
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
    int count = game->aiController.driverCount;
    double trackLength = game->road.trackLength;
    
    for (int i = 0; i < count; i++) {
        AIDriver* driverA = &game->aiController.drivers[i];
        // Ensure a valid segment for driverA
        Segment* segA = findSegment(game, driverA->z);
        if (!segA) continue;
        
        double aiAWidth = (driverA->sprite) ? driverA->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;
        double aLeft = driverA->offset - aiAWidth / 2;
        double aRight = driverA->offset + aiAWidth / 2;
        
        for (int j = i + 1; j < count; j++) {
            AIDriver* driverB = &game->aiController.drivers[j];
            Segment* segB = findSegment(game, driverB->z);
            if (!segB) continue;
            
            double aiBWidth = (driverB->sprite) ? driverB->sprite->w * SPRITE_SCALE : 80 * SPRITE_SCALE;
            double bLeft = driverB->offset - aiBWidth / 2;
            double bRight = driverB->offset + aiBWidth / 2;
            
            // Calculate longitudinal distance with track wrapping.
            double deltaZ = driverA->z - driverB->z;
            if (deltaZ < -trackLength / 2) deltaZ += trackLength;
            else if (deltaZ > trackLength / 2) deltaZ -= trackLength;
            double absDeltaZ = fabs(deltaZ);
            
            // Only consider collisions if the drivers are close enough in z.
            if (absDeltaZ < LONGITUDINAL_COLLISION_THRESHOLD) {
                bool lateralOverlap = (aRight > bLeft && aLeft < bRight);
                if (lateralOverlap) {
                    // Define the push amount
                    double pushAmount = 0.5;
                    
                    // For driverA: set push if not already in a push state.
                    if (!driverA->isBeingPushed) {
                        driverA->isBeingPushed = true;
                        driverA->pushStartOffset = driverA->offset;
                        if (driverA->offset < driverB->offset)
                            driverA->pushTargetOffset = driverA->offset - pushAmount;
                        else
                            driverA->pushTargetOffset = driverA->offset + pushAmount;
                        driverA->pushElapsed = 0.0;
                    }
                    
                    // For driverB: similarly set push.
                    if (!driverB->isBeingPushed) {
                        driverB->isBeingPushed = true;
                        driverB->pushStartOffset = driverB->offset;
                        if (driverB->offset < driverA->offset)
                            driverB->pushTargetOffset = driverB->offset - pushAmount;
                        else
                            driverB->pushTargetOffset = driverB->offset + pushAmount;
                        driverB->pushElapsed = 0.0;
                    }
                    
                    // trigger collision animations.
                    driverA->collisionAnim.active = true;
                    driverA->collisionAnim.finished = false;
                    driverB->collisionAnim.active = true;
                    driverB->collisionAnim.finished = false;
                    
                    // Mark both as in collision.
                    driverA->inCollision = true;
                    driverB->inCollision = true;
                }
            }
        }
    }
}

#ifdef DEBUG_MODE
void debugSideCollision(
    double aiScale,
    double sideCollisionScaleThreshold,
    bool lateralOverlap,
    double gamePlayerX,
    double driverOffset,
    double aiLeft,
    double aiRight,
    double absDeltaZ,
    double segmentLength,
    double sideCollisionZMultiplier,
    double playerW,
    double sideCollisionXMultiplier)
{
    // Check if the AI is large enough for side collisions.
    bool sideCollisionsEnabled = (aiScale > sideCollisionScaleThreshold);
    SDL_Log("[DEBUG] Side Collisions: %s (AI Scale: %.3f vs Threshold: %.3f)",
            sideCollisionsEnabled ? "ENABLED" : "DISABLED", aiScale, sideCollisionScaleThreshold);

    // Log lateral overlap info.
    SDL_Log("[DEBUG] Lateral Overlap: %s (PlayerX: %.3f, AI Offset: %.3f, AI Bounds: [%.3f, %.3f])",
            lateralOverlap ? "DETECTED" : "NOT DETECTED", gamePlayerX, driverOffset, aiLeft, aiRight);

    // Calculate and log the Z-distance condition.
    double maxAllowedDeltaZ = segmentLength * sideCollisionZMultiplier;
    SDL_Log("[DEBUG] Z Condition: %s (absDeltaZ: %.3f vs MaxAllowed: %.3f)",
            (absDeltaZ < maxAllowedDeltaZ) ? "MET" : "NOT MET", absDeltaZ, maxAllowedDeltaZ);

    // Calculate and log the lateral (X) offset condition.
    double requiredLateralOffset = playerW * sideCollisionXMultiplier;
    double actualLateralOffset = fabs(gamePlayerX - driverOffset);
    SDL_Log("[DEBUG] Lateral Offset Condition: %s (Actual: %.3f vs Required: %.3f)",
            (actualLateralOffset > requiredLateralOffset) ? "MET" : "NOT MET", actualLateralOffset, requiredLateralOffset);

    // Finally, combine all conditions.
    bool isSideCollision = sideCollisionsEnabled && lateralOverlap &&
                           (absDeltaZ < maxAllowedDeltaZ) &&
                           (actualLateralOffset > requiredLateralOffset);
    SDL_Log("[DEBUG] Final Side Collision: %s", isSideCollision ? "TRIGGERED" : "NOT TRIGGERED");
}
#endif  // DEBUG_MODE