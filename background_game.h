#ifndef BACKGROUND_GAME_H
#define BACKGROUND_GAME_H

#include "game.h"

// Prototypes for functions that require Game*
void updateBackgroundSlidingWindow(Game* game, SlidingWindow* window, float deltaTime, double playerSpeed, double roadCurvature);
void startBackgroundTransition(Game* game, SlidingWindow* window, int targetIndex);
void advanceToNextState(Game* game, SlidingWindow* window);
void updateLayerTransitions(Game* game, BackgroundCycleState* currentState, BackgroundCycleState* targetState,
                            float deltaTime, double playerSpeed, double roadCurvature);

#endif // BACKGROUND_GAME_H