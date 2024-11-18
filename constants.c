// constants.c
#include "constants.h"

// Helper function to convert hex color code to Color struct
Color hexToColor(const char* hexString) {
    Color color = {0, 0, 0, 255}; // Default alpha to 255
    unsigned int hex;
    sscanf(hexString + 1, "%06x", &hex);
    color.r = (hex >> 16) & 0xFF;
    color.g = (hex >> 8) & 0xFF;
    color.b = hex & 0xFF;
    return color;
}

// Color constants
const Color COLOR_SKY  = {114, 215, 238, 255}; // #72D7EE
const Color COLOR_TREE = {0, 81, 8, 255};      // #005108
const Color COLOR_FOG  = {0, 81, 8, 255};      // #005108

// Color schemes
const ColorScheme COLOR_LIGHT = {
    .road   = {107, 107, 107, 255}, // #6B6B6B
    .grass  = {16, 170, 16, 255},   // #10AA10
    .rumble = {85, 85, 85, 255},    // #555555
    .lane   = {204, 204, 204, 255}  // #CCCCCC
};

const ColorScheme COLOR_DARK = {
    .road   = {105, 105, 105, 255}, // #696969
    .grass  = {0, 154, 0, 255},     // #009A00
    .rumble = {187, 187, 187, 255}, // #BBBBBB
    .lane   = {0, 0, 0, 0}          // transparent
};

const ColorScheme COLOR_START = {
    .road   = {255, 255, 255, 255}, // 'white'
    .grass  = {255, 255, 255, 255}, // 'white'
    .rumble = {255, 255, 255, 255}, // 'white'
    .lane   = {0, 0, 0, 0}          // transparent
};

const ColorScheme COLOR_FINISH = {
    .road   = {170, 0, 0, 255},       // 'black'
    .grass  = {170, 0, 0, 255},       // 'black'
    .rumble = {170, 0, 0, 255},       // 'black'
    .lane   = {0, 0, 0, 0}          // transparent
};

// Background layer LODs (x, y, width, height)
const BackgroundLayer BACKGROUND_HILLS = {
    .lowLOD = {5, 0, 1280, 480},
    .midLOD  = {5, 1440, 1280, 480},
    .highLOD  = {5, 1920, 1280, 480}
};

const BackgroundLayer BACKGROUND_TREES = {
    .lowLOD = {5, 960, 1280, 480},
    .midLOD  = {5, 2400, 1280, 480},
    .highLOD  = {5, 2880, 1280, 480}
};

const BackgroundLayer BACKGROUND_SKY = {
    .highLOD = {5, 480, 1280, 480}, // Sky will remain static
    .midLOD  = {5, 480, 1280, 480}, // Same as high LOD
    .lowLOD  = {5, 480, 1280, 480}  // Same as high LOD
};

const BackgroundLayer BACKGROUND_HOUSES = {
    .lowLOD = {5, 3360, 1280, 480},
    .midLOD = {5, 3840, 1280, 480},
    .highLOD = {5, 4320, 1280, 480}
};

// Offsets for each texture in each layer cycle state
static const Offset HILLS_LOW_OFFSET = {0, -75};
static const Offset HILLS_MID_OFFSET = {0, 0};
static const Offset HILLS_HIGH_OFFSET = {0, 0};

static const Offset TREES_LOW_OFFSET = {0, 0};
static const Offset TREES_MID_OFFSET = {0, -145};
static const Offset TREES_HIGH_OFFSET = {0, 0};

static const Offset HOUSES_LOW_OFFSET = {0, -500};
static const Offset HOUSES_MID_OFFSET = {0, -400};
static const Offset HOUSES_HIGH_OFFSET = {0, -350};

// Define background cycle states with specific textures and offsets
const BackgroundCycleState BACKGROUND_CYCLE_STATES[3] = {
    { &BACKGROUND_HILLS.lowLOD, &HILLS_LOW_OFFSET, &BACKGROUND_TREES.midLOD, &TREES_MID_OFFSET, &BACKGROUND_HOUSES.highLOD, &HOUSES_HIGH_OFFSET },
    { &BACKGROUND_HOUSES.lowLOD, &HOUSES_LOW_OFFSET, &BACKGROUND_HILLS.midLOD, &HILLS_MID_OFFSET, &BACKGROUND_TREES.highLOD, &TREES_HIGH_OFFSET },
    { &BACKGROUND_TREES.lowLOD, &TREES_LOW_OFFSET, &BACKGROUND_HOUSES.midLOD, &HOUSES_MID_OFFSET, &BACKGROUND_HILLS.highLOD, &HILLS_HIGH_OFFSET }
};

// Sprite constants
const Sprite SPRITE_PALM_TREE              = {5,    5,  215, 540};
const Sprite SPRITE_BILLBOARD08            = {230,  5,  385, 265};
const Sprite SPRITE_TREE1                  = {625,  5,  360, 360};
const Sprite SPRITE_DEAD_TREE1             = {5,  555,  135, 332};
const Sprite SPRITE_BILLBOARD09            = {150, 555,  328, 282};
const Sprite SPRITE_BOULDER3               = {230, 280,  320, 220};
const Sprite SPRITE_COLUMN                 = {995,   5,  200, 315};
const Sprite SPRITE_BILLBOARD01            = {625, 375,  300, 170};
const Sprite SPRITE_BILLBOARD06            = {488, 555,  298, 190};
const Sprite SPRITE_BILLBOARD05            = {5,   897,  298, 190};
const Sprite SPRITE_BILLBOARD07            = {313, 897,  298, 190};
const Sprite SPRITE_BOULDER2               = {621, 897,  298, 140};
const Sprite SPRITE_TREE2                  = {1205,  5,  282, 295};
const Sprite SPRITE_BILLBOARD04            = {1205,310,  268, 170};
const Sprite SPRITE_DEAD_TREE2             = {1205,490,  150, 260};
const Sprite SPRITE_BOULDER1               = {1205,760,  168, 248};
const Sprite SPRITE_BUSH1                  = {5,  1097,  240, 155};
const Sprite SPRITE_CACTUS                 = {929, 897,  235, 118};
const Sprite SPRITE_BUSH2                  = {255,1097,  232, 152};
const Sprite SPRITE_BILLBOARD03            = {5,  1262,  230, 220};
const Sprite SPRITE_BILLBOARD02            = {245,1262,  215, 220};
const Sprite SPRITE_STUMP                  = {995, 330,  195, 140};
const Sprite SPRITE_SEMI                   = {1365,490,  122, 144};
const Sprite SPRITE_TRUCK                  = {1365,644,  100,  78};
const Sprite SPRITE_CAR03                  = {1383,760,   88,  55};
const Sprite SPRITE_CAR02                  = {1383,825,   80,  59};
const Sprite SPRITE_CAR04                  = {1383,894,   80,  57};
const Sprite SPRITE_CAR01                  = {1205,1018,  80,  56};
const Sprite SPRITE_PLAYER_UPHILL_LEFT     = {1383,961,   80,  45};
const Sprite SPRITE_PLAYER_UPHILL_STRAIGHT = {1295,1018,  80,  45};
const Sprite SPRITE_PLAYER_UPHILL_RIGHT    = {1385,1018,  80,  45};
const Sprite SPRITE_PLAYER_LEFT            = {995, 480,   80,  41};
const Sprite SPRITE_PLAYER_STRAIGHT        = {1085,480,   80,  41};
const Sprite SPRITE_PLAYER_RIGHT           = {995, 531,   80,  41};

// Arrays of sprites
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