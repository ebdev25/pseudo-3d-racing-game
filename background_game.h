#ifndef BACKGROUND_GAME_H
#define BACKGROUND_GAME_H

#include "game.h"

// Prototypes for functions that require Game*
void setupTargetStateUnwrapped(BackgroundCycleState* target, int screenWidth);
void clampLayerOffsetsToVisibleRange(Game* game);
void updateBackgroundSlidingWindow(Game* game, SlidingWindow* window, float deltaTime, double playerSpeed, double roadCurvature);
void startBackgroundTransition(Game* game, SlidingWindow* window, int targetIndex);
void advanceToNextState(Game* game, SlidingWindow* window);
void updateParallax(Game* game, SlidingWindow* window, BackgroundCycleState* state, double playerSpeed,
                    double roadCurvature, double elevation, double position,
                    double previousPosition, double dt, double horizonY, bool transitionHappening);
void updateTransitionColors(Game* game, SlidingWindow* window, float deltaTime);
void updateLayerTransitions(Game* game, BackgroundCycleState* currentState, BackgroundCycleState* targetState,
                            float deltaTime, double playerSpeed, double roadCurvature);

#endif // BACKGROUND_GAME_H