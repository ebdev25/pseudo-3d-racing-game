#ifndef COLOR_H
#define COLOR_H

#include <SDL2/SDL.h>

// Unified Color struct for both purposes
typedef struct {
    Uint8 r, g, b, a;
} Color;

#endif // COLOR_H