// util.c
#include "util.h"
#include <math.h>

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

float easeInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
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

    // Calculate the scale factor based on distance from the camera (Z-axis depth)
    // As depth (camera.z) increases, scale decreases, creating perspective effect
    p->scale = cameraDepth / p->camera.z;

    // Project the X coordinate onto screen space
    // Center the X position in the middle of the screen, then apply scaling based on distance
    p->screen.x = round((width / 2.0) + (p->scale * p->camera.x * width / 2.0));

    // Project the Y coordinate onto screen space
    // Center the Y position vertically on the screen, then apply scaling based on distance
    p->screen.y = round((height / 2.0) - (p->scale * p->camera.y * height / 2.0));

    // Project the road width onto screen space, scaled based on depth to create perspective
    p->screen.w = round(p->scale * roadWidth * width / 2.0);
}

// Calculates the percentage (0-1) of n within the total length
double percentRemaining(double n, double total) {
    return fmod(n, total) / total;
}

// Checks if two objects overlap based on their positions and widths
int overlap(double x1, double w1, double x2, double w2, double percent) {
    // Calculate half of the combined widths, scaled by the overlap threshold percentage
    double half = (w1 + w2) * (percent / 2.0);

    // Check if the distance between x1 and x2 is less than the calculated half width
    // If true, it means the objects are within the overlap threshold
    return fabs(x1 - x2) < half;
}

float lerp_float(float a, float b, float t) {
    return a + t * (b - a);
}