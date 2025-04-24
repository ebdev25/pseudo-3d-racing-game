// level.h
#ifndef LEVEL_H
#define LEVEL_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "background.h"
#include "color.h"

typedef struct {
    Color lightGrass;  // hex #xx for the 'light' grass color
    Color darkGrass;   // hex #xx for the 'dark' grass color
} GrassColorOverrides;

// Structure for resource paths
typedef struct {
    char* backgroundTexture;
    char* spriteSheet;
    char* musicFile;
} LevelResources;

// Structure for background data
typedef struct {
    BackgroundCycleState* cycleStates; // Array of cycle states
    int cycleStateCount;                // Number of cycle states
} LevelBackground;

// Each RoadCommand stores one line from the "commands" array
typedef struct {
    char* type;     // e.g. "straight", "hill", "curve", etc
    float* params;  // dynamic array of numeric parameters
    int paramCount; // how many params were read from JSON
} RoadCommand;

// RoadData holds all road commands for this level
typedef struct {
    RoadCommand* commands; 
    int commandCount;
} LevelRoadData;

// Level structure
typedef struct {
    char* levelName;
    LevelResources resources;
    LevelBackground background;
    LevelRoadData roadData;
    GrassColorOverrides grassOverrides;
} Level;

#endif // LEVEL_H
