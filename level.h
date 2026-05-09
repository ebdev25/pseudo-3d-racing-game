// level.h
#ifndef LEVEL_H
#define LEVEL_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "background.h"
#include "color.h"
#include "constants.h"

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

/* Roadside props: segmentIndex < 0 counts back from end (e.g. -25 -> last 25th segment). */
typedef struct {
    const Sprite* sprite;
    int segmentIndex;
    double offset;
} LevelSceneryItem;

typedef struct {
    const Sprite* sprite;
    double offset;
} LevelSceneryRepeatPlacement;

/* fromSegment / toSegment may be negative (resolved against segment count in resetSprites). */
typedef struct {
    int fromSegment;
    int toSegment;
    int step;
    LevelSceneryRepeatPlacement* placements;
    int placementCount;
} LevelSceneryRepeatRule;

typedef struct {
    LevelSceneryItem* items;
    int itemCount;
    LevelSceneryRepeatRule* repeatRules;
    int repeatRuleCount;
} LevelSceneryData;

// Level structure
typedef struct {
    char* levelName;
    LevelResources resources;
    LevelBackground background;
    LevelRoadData roadData;
    GrassColorOverrides grassOverrides;
} Level;

#endif // LEVEL_H
