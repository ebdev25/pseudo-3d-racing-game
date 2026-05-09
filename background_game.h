#ifndef BACKGROUND_GAME_H
#define BACKGROUND_GAME_H

#include "game.h"

void setupTargetStateUnwrapped(BackgroundCycleState* target, int screenWidth);
void startBackgroundTransition(Game* game, SlidingWindow* window, int targetIndex);
void advanceToNextState(Game* game, SlidingWindow* window);
void updateParallax(Game* game, SlidingWindow* window, BackgroundCycleState* state, double playerSpeed,
                    double roadCurvature, double elevation, double position,
                    double previousPosition, double dt, double horizonY, bool transitionHappening);
void updateTransitionColors(Game* game, SlidingWindow* window, float deltaTime);
void resetSlidingWindow(SlidingWindow* window);
void updateBackgroundTransition(Game* game, double dt, double startPosition);

#endif // BACKGROUND_GAME_H