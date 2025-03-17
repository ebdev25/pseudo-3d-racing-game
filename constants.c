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
const Color COLOR_FOG  = {0, 81, 8, 255};      // Fog green

// Color schemes
ColorScheme COLOR_LIGHT = {
    .road   = {107, 107, 107, 255}, // Gray
    .grass  = {16, 170, 16, 255},   // Bright green
    .rumble = {85, 85, 85, 255},    // Dark gray
    .lane   = {204, 204, 204, 255}  // Light gray
};

ColorScheme COLOR_DARK = {
    .road   = {105, 105, 105, 255}, // Darker gray
    .grass  = {0, 154, 0, 255},     // Dark green
    .rumble = {187, 187, 187, 255}, // Light gray
    .lane   = {0, 0, 0, 0}          // Transparent
};

ColorScheme COLOR_START = {
    .road   = {255, 255, 255, 255}, // White
    .grass  = {255, 255, 255, 255}, // White
    .rumble = {255, 255, 255, 255}, // White
    .lane   = {0, 0, 0, 0}          // Transparent
};

ColorScheme COLOR_FINISH = {
    .road   = {170, 0, 0, 255}, // Black
    .grass  = {170, 0, 0, 255}, // Black
    .rumble = {170, 0, 0, 255}, // Black
    .lane   = {0, 0, 0, 0}      // Transparent
};

// Sprite definitions
/*
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
*/

const Sprite SPRITE_BILLBOARD08 = {230, 5, 385, 265};
const Sprite SPRITE_BILLBOARD09 = {150, 555, 328, 282};
const Sprite SPRITE_BILLBOARD01 = {625, 375, 300, 170};
const Sprite SPRITE_BILLBOARD06 = {488, 555, 298, 190};
const Sprite SPRITE_BILLBOARD05 = {5, 897, 298, 190};
const Sprite SPRITE_BILLBOARD07 = {313, 897, 298, 190};
const Sprite SPRITE_BILLBOARD04 = {1205, 310, 268, 170};
const Sprite SPRITE_BILLBOARD03 = {5, 1262, 230, 220};
const Sprite SPRITE_BILLBOARD02 = {245, 1262, 215, 220};

const Sprite SPRITE_PLAYER_UPHILL_LEFT = {1383, 961, 80, 45};
const Sprite SPRITE_PLAYER_UPHILL_STRAIGHT = {1295, 1018, 80, 45};
const Sprite SPRITE_PLAYER_UPHILL_RIGHT = {1385, 1018, 80, 45};
const Sprite SPRITE_PLAYER_LEFT = {995, 480, 80, 41};
const Sprite SPRITE_PLAYER_STRAIGHT = {1085, 480, 80, 41};
const Sprite SPRITE_PLAYER_RIGHT = {995, 531, 80, 41};

// New AI Opponent sprites, same coords as player but from separate texture
const Sprite SPRITE_AI_UPHILL_LEFT     = {1383, 961, 80, 45};
const Sprite SPRITE_AI_UPHILL_STRAIGHT = {1295, 1018,80, 45};
const Sprite SPRITE_AI_UPHILL_RIGHT    = {1385, 1018,80, 45};
const Sprite SPRITE_AI_LEFT            = {995,  480, 80, 41};
const Sprite SPRITE_AI_STRAIGHT        = {1085, 480, 80, 41};
const Sprite SPRITE_AI_RIGHT           = {995,  531, 80, 41};

const Sprite SPRITE_AI_UPHILL_LEFT_OPP_2 = {222, 519, 80, 45};
const Sprite SPRITE_AI_UPHILL_STRAIGHT_OPP_2 = {134, 576, 80, 45};
const Sprite SPRITE_AI_UPHILL_RIGHT_OPP_2 = {224, 576, 80, 45};
const Sprite SPRITE_AI_LEFT_OPP_2 = {35, 465, 77, 41};
const Sprite SPRITE_AI_STRAIGHT_OPP_2 = {123, 465, 78, 41};
const Sprite SPRITE_AI_RIGHT_OPP_2 = {32, 516, 77, 41};

const Sprite SPRITE_AI_UPHILL_LEFT_OPP_3 = {216, 779, 80, 45};
const Sprite SPRITE_AI_UPHILL_STRAIGHT_OPP_3 = {128, 836, 80, 45};
const Sprite SPRITE_AI_UPHILL_RIGHT_OPP_3 = {218, 836, 80, 45};
const Sprite SPRITE_AI_LEFT_OPP_3 = {29, 725, 77, 41};
const Sprite SPRITE_AI_STRAIGHT_OPP_3 = {117, 725, 78, 41};
const Sprite SPRITE_AI_RIGHT_OPP_3 = {26, 776, 77, 41};

const Sprite SPRITE_AI_COLLISION_FRAME1 = {0, 0, 80, 45};
const Sprite SPRITE_AI_COLLISION_FRAME2 = {80, 0, 80, 45};
const Sprite SPRITE_AI_COLLISION_FRAME3 = {160, 0, 80, 45};
const Sprite SPRITE_AI_COLLISION_FRAME4 = {240, 0, 80, 45};
const Sprite SPRITE_AI_COLLISION_FRAME5 = {320, 0, 80, 45};

// Racing light coordinates mapping to sprites on sheet
const Sprite SPRITE_LIGHT_FRAME1 = {1, 1, 316, 233};
const Sprite SPRITE_LIGHT_FRAME2 = {318, 1, 316, 233};
const Sprite SPRITE_LIGHT_FRAME3 = {635, 1, 316, 233};
const Sprite SPRITE_LIGHT_FRAME4 = {952, 1, 316, 233};
const Sprite SPRITE_LIGHT_FRAME5 = {1, 235, 316, 233};

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
/*
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
*/