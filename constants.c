#include "constants.h"

// Helper function to convert hex color code to Color struct
static Color hexToColor(const char* hexString) {
    Color color = {0, 0, 0, 255}; // Default alpha to 255
    unsigned int hex;
    sscanf(hexString + 1, "%06x", &hex);
    color.r = (hex >> 16) & 0xFF;
    color.g = (hex >> 8) & 0xFF;
    color.b = hex & 0xFF;
    return color;
}

// Color constants
const Color COLOR_SKY  = {114, 215, 238, 255}; // Sky blue
const Color COLOR_TREE = {0, 81, 8, 255};      // Dark green
const Color COLOR_FOG  = {0, 81, 8, 255};      // Fog green

// Color schemes
const ColorScheme COLOR_LIGHT = {
    .road   = {107, 107, 107, 255}, // Gray
    .grass  = {16, 170, 16, 255},   // Bright green
    .rumble = {85, 85, 85, 255},    // Dark gray
    .lane   = {204, 204, 204, 255}  // Light gray
};

const ColorScheme COLOR_DARK = {
    .road   = {105, 105, 105, 255}, // Darker gray
    .grass  = {0, 154, 0, 255},     // Dark green
    .rumble = {187, 187, 187, 255}, // Light gray
    .lane   = {0, 0, 0, 0}          // Transparent
};

const ColorScheme COLOR_START = {
    .road   = {255, 255, 255, 255}, // White
    .grass  = {255, 255, 255, 255}, // White
    .rumble = {255, 255, 255, 255}, // White
    .lane   = {0, 0, 0, 0}          // Transparent
};

const ColorScheme COLOR_FINISH = {
    .road   = {170, 0, 0, 255}, // Black
    .grass  = {170, 0, 0, 255}, // Black
    .rumble = {170, 0, 0, 255}, // Black
    .lane   = {0, 0, 0, 0}      // Transparent
};

// Background layers for LOD
const BackgroundLayer BACKGROUND_TREES = {
    .highLOD  = {0, 0, 3870, 350},
    .midLOD   = {0, 350, 3870, 350},
    .lowLOD   = {0, 700, 3870, 350}
};

const BackgroundLayer BACKGROUND_HILLS = {
    .highLOD  = {0, 1050, 3870, 350},
    .midLOD   = {0, 1400, 3870, 350},
    .lowLOD   = {0, 1750, 3870, 350}
};

const BackgroundLayer BACKGROUND_MOUNTAINS = {
    .highLOD  = {0, 2100, 3870, 350},
    .midLOD   = {0, 2450, 3870, 350},
    .lowLOD   = {0, 2800, 3870, 350}
};

const BackgroundLayer BACKGROUND_SKY = {
    .highLOD  = {0, 3150, 3870, 1000},
    .midLOD   = {0, 3150, 3870, 1000},
    .lowLOD   = {0, 3150, 3870, 1000}
};

// Offsets for each texture
const Offset HILLS_LOW_OFFSET     = {0, 100};
const Offset HILLS_MID_OFFSET     = {0, 100};
const Offset HILLS_HIGH_OFFSET    = {0, 100};

const Offset TREES_LOW_OFFSET     = {0, 100};
const Offset TREES_MID_OFFSET     = {0, 155};
const Offset TREES_HIGH_OFFSET    = {0, 100};

const Offset MOUNTAINS_LOW_OFFSET  = {0, 100};
const Offset MOUNTAINS_MID_OFFSET  = {0, 100};
const Offset MOUNTAINS_HIGH_OFFSET = {0, 100};

const Offset SKY_OFFSET = {0, 0};

// Background cycle states
const BackgroundCycleState BACKGROUND_CYCLE_STATES[3] = {
    // Phase 0
    {
        &BACKGROUND_TREES.lowLOD,      &TREES_LOW_OFFSET,
        &BACKGROUND_TREES.midLOD,      &TREES_MID_OFFSET,
        &BACKGROUND_TREES.highLOD,     &TREES_HIGH_OFFSET
    },
    // Phase 1
    {
        &BACKGROUND_HILLS.lowLOD,      &HILLS_LOW_OFFSET,
        &BACKGROUND_HILLS.midLOD,      &HILLS_MID_OFFSET,
        &BACKGROUND_HILLS.highLOD,     &HILLS_HIGH_OFFSET
    },
    // Phase 2
    {
        &BACKGROUND_MOUNTAINS.lowLOD,  &MOUNTAINS_LOW_OFFSET,
        &BACKGROUND_MOUNTAINS.midLOD,  &MOUNTAINS_MID_OFFSET,
        &BACKGROUND_MOUNTAINS.highLOD, &MOUNTAINS_HIGH_OFFSET
    }
};

// Sprite definitions
const Sprite SPRITE_PALM_TREE = {5, 5, 215, 540};
const Sprite SPRITE_BILLBOARD08 = {230, 5, 385, 265};
const Sprite SPRITE_TREE1 = {625, 5, 360, 360};
const Sprite SPRITE_DEAD_TREE1 = {5, 555, 135, 332};
const Sprite SPRITE_BILLBOARD09 = {150, 555, 328, 282};
const Sprite SPRITE_BOULDER3 = {230, 280, 320, 220};
const Sprite SPRITE_COLUMN = {995, 5, 200, 315};
const Sprite SPRITE_BILLBOARD01 = {625, 375, 300, 170};
const Sprite SPRITE_BILLBOARD06 = {488, 555, 298, 190};
const Sprite SPRITE_BILLBOARD05 = {5, 897, 298, 190};
const Sprite SPRITE_BILLBOARD07 = {313, 897, 298, 190};
const Sprite SPRITE_BOULDER2 = {621, 897, 298, 140};
const Sprite SPRITE_TREE2 = {1205, 5, 282, 295};
const Sprite SPRITE_BILLBOARD04 = {1205, 310, 268, 170};
const Sprite SPRITE_DEAD_TREE2 = {1205, 490, 150, 260};
const Sprite SPRITE_BOULDER1 = {1205, 760, 168, 248};
const Sprite SPRITE_BUSH1 = {5, 1097, 240, 155};
const Sprite SPRITE_CACTUS = {929, 897, 235, 118};
const Sprite SPRITE_BUSH2 = {255, 1097, 232, 152};
const Sprite SPRITE_BILLBOARD03 = {5, 1262, 230, 220};
const Sprite SPRITE_BILLBOARD02 = {245, 1262, 215, 220};
const Sprite SPRITE_STUMP = {995, 330, 195, 140};
const Sprite SPRITE_SEMI = {1365, 490, 122, 144};
const Sprite SPRITE_TRUCK = {1365, 644, 100, 78};
const Sprite SPRITE_CAR03 = {1383, 760, 88, 55};
const Sprite SPRITE_CAR02 = {1383, 825, 80, 59};
const Sprite SPRITE_CAR04 = {1383, 894, 80, 57};
const Sprite SPRITE_CAR01 = {1205, 1018, 80, 56};
const Sprite SPRITE_PLAYER_UPHILL_LEFT = {1383, 961, 80, 45};
const Sprite SPRITE_PLAYER_UPHILL_STRAIGHT = {1295, 1018, 80, 45};
const Sprite SPRITE_PLAYER_UPHILL_RIGHT = {1385, 1018, 80, 45};
const Sprite SPRITE_PLAYER_LEFT = {995, 480, 80, 41};
const Sprite SPRITE_PLAYER_STRAIGHT = {1085, 480, 80, 41};
const Sprite SPRITE_PLAYER_RIGHT = {995, 531, 80, 41};

// Sprite arrays
const Sprite* SPRITES_BILLBOARDS[BILLBOARD_COUNT] = {
    &SPRITE_BILLBOARD01,
    &SPRITE_BILLBOARD02,
    &SPRITE_BILLBOARD03,
    &SPRITE_BILLBOARD04,
    &SPRITE_BILLBOARD05,
    &SPRITE_BILLBOARD06,
    &SPRITE_BILLBOARD07,
    &SPRITE_BILLBOARD08,
    &SPRITE_BILLBOARD09
};

const Sprite* SPRITES_PLANTS[PLANT_COUNT] = {
    &SPRITE_TREE1,
    &SPRITE_TREE2,
    &SPRITE_DEAD_TREE1,
    &SPRITE_DEAD_TREE2,
    &SPRITE_PALM_TREE,
    &SPRITE_BUSH1,
    &SPRITE_BUSH2,
    &SPRITE_CACTUS,
    &SPRITE_STUMP,
    &SPRITE_BOULDER1,
    &SPRITE_BOULDER2,
    &SPRITE_BOULDER3
};

const Sprite* SPRITES_CARS[CAR_COUNT] = {
    &SPRITE_CAR01,
    &SPRITE_CAR02,
    &SPRITE_CAR03,
    &SPRITE_CAR04,
    &SPRITE_SEMI,
    &SPRITE_TRUCK
};

// Create and initialize a SlidingWindow instance
SlidingWindow* createSlidingWindow() {
    SlidingWindow* window = (SlidingWindow*)malloc(sizeof(SlidingWindow));
    if (!window) {
        fprintf(stderr, "Failed to allocate memory for SlidingWindow\n");
        exit(EXIT_FAILURE);
    }
    window->currentStateIndex = 0;
    window->targetStateIndex = 0;
    window->progress = 0.0f;
    window->transitioning = false;
    window->movingForward = true;
    return window;
}

// Start a transition to the next or previous cycle state
void startTransition(SlidingWindow* window, bool forward) {
    if (window->transitioning) return; // Prevent overlapping transitions
    window->transitioning = true;
    window->progress = 0.0f;
    window->movingForward = forward;
    if (forward) {
        window->targetStateIndex = (window->currentStateIndex + 1) % 3;
    } else {
        window->targetStateIndex = (window->currentStateIndex - 1 + 3) % 3;
    }
}

// Finalize the transition by updating the current state index
void finalizeTransition(SlidingWindow* window) {
    window->currentStateIndex = window->targetStateIndex;
    window->transitioning = false;
    window->progress = 0.0f;
}
