// util.c
#include "util.h"
#include <math.h>

// Returns the current time in seconds since the program started
double timestamp() {
    return SDL_GetTicks() / 1000.0;
}

// Constrains a value within a specified range [min, max]
double limit(double value, double min, double max) {
    return fmax(min, fmin(value, max));
}

// Calculates the new velocity after applying acceleration over time dt
double accelerate(double v, double accel, double dt) {
    return v + (accel * dt);
}

// Linearly interpolates between two values (a and b) by a percentage (0-1)
double interpolate(double a, double b, double percent) {
    return a + (b - a) * percent;
}

// Performs quadratic easing in for smoother start of interpolation from a to b
double easeIn(double a, double b, double percent) {
    return a + (b - a) * pow(percent, 2);
}

// Performs quadratic easing out for smoother end of interpolation from a to b
double easeOut(double a, double b, double percent) {
    return a + (b - a) * (1 - pow(1 - percent, 2));
}

// Performs quadratic easing in and out for smoother start and end of interpolation
double easeInOut(double a, double b, double percent) {
    return a + (b - a) * ((-cos(percent * M_PI) / 2) + 0.5);
}

// Computes fog intensity based on distance and density for exponential fog effect
double exponentialFog(double distance, double density) {
    return 1.0 / (exp(pow(distance, 2) * density));
}

// Increments a value by a specified amount, wrapping around max if it exceeds max
double increase(double start, double increment, double max) {
    double result = start + increment;
    while (result >= max)
        result -= max;
    while (result < 0)
        result += max;
    return result;
}

// Projects a 3D world point onto 2D screen space based on camera position and perspective settings
void project(Point* p, double cameraX, double cameraY, double cameraZ, double cameraDepth, int width, int height, double roadWidth) {
    
    // Translate the point's world coordinates to camera coordinates by subtracting the camera's position
    p->camera.x = p->world.x - cameraX;  // X-axis translation relative to the camera
    p->camera.y = p->world.y - cameraY;  // Y-axis translation relative to the camera
    p->camera.z = p->world.z - cameraZ;  // Z-axis translation (depth) relative to the camera

    // Calculate the scale factor based on distance from the camera (Z-axis depth).
    // As depth (camera.z) increases, scale decreases, creating perspective effect.
    p->scale = cameraDepth / p->camera.z;

    // Project the X coordinate onto screen space.
    // Center the X position in the middle of the screen, then apply scaling based on distance.
    p->screen.x = round((width / 2.0) + (p->scale * p->camera.x * width / 2.0));

    // Project the Y coordinate onto screen space.
    // Center the Y position vertically on the screen, then apply scaling based on distance.
    p->screen.y = round((height / 2.0) - (p->scale * p->camera.y * height / 2.0));

    // Project the road width onto screen space, scaled based on depth to create perspective.
    p->screen.w = round(p->scale * roadWidth * width / 2.0);
}

// Calculates the percentage (0-1) of n within the total length
double percentRemaining(double n, double total) {
    return fmod(n, total) / total;
}