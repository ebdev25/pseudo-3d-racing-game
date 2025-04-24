#ifndef UTIL_H
#define UTIL_H

#include <SDL2/SDL.h>
#include "constants.h"

double limit(double value, double min, double max);
double accelerate(double v, double accel, double dt);
double interpolate(double a, double b, double percent);
double easeIn(double a, double b, double percent);
double easeOut(double a, double b, double percent);
double easeInOut(double a, double b, double percent);
float easeInOutCubic(float t);
double increase(double start, double increment, double max);
void project(Point* p, double cameraX, double cameraY, double cameraZ, double cameraDepth, int width, int height, double roadWidth);
double percentRemaining(double n, double total);
int overlap(double x1, double w1, double x2, double w2, double percent);
float lerp_float(float a, float b, float t);
#endif // UTIL_H