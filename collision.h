#ifndef COLLISION_H
#define COLLISION_H

#include "game.h"

void checkAIDriverBillboardCollisions(Game* game, AIDriver* driver);
void checkPlayerAIOpponentCollisions(Game* game);
void checkAIDriverCollisions(Game* game);

#endif // COLLISION_H