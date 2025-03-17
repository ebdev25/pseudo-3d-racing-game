#ifndef COLLISION_H
#define COLLISION_H

#include "game.h"

void checkAIDriverBillboardCollisions(Game* game, AIDriver* driver);
void checkPlayerAIOpponentCollisions(Game* game);
void checkAIDriverCollisions(Game* game);

#endif // COLLISION_H

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
    double sideCollisionXMultiplier);
#else
#define debugSideCollision(...) ((void)0)
#endif