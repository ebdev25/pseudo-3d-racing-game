#ifndef UTIL_H
#define UTIL_H

#include <SDL2/SDL.h>
#include "constants.h"

double timestamp();
double limit(double value, double min, double max);
double accelerate(double v, double accel, double dt);
double interpolate(double a, double b, double percent);
double easeIn(double a, double b, double percent);
double easeOut(double a, double b, double percent);
double easeInOut(double a, double b, double percent);
double exponentialFog(double distance, double density);
double increase(double start, double increment, double max);
void project(Point* p, double cameraX, double cameraY, double cameraZ, double cameraDepth, int width, int height, double roadWidth);
double percentRemaining(double n, double total);

#endif // UTIL_H