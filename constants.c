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

const Sprite SPRITE_PALM_TREE_LEFT = {656, 909, 215, 541};
const Sprite SPRITE_PALM_TREE_RIGHT = {5, 5, 215, 540};
const Sprite SPRITE_ROCK = {910, 711, 125, 123};
const Sprite SPRITE_UMBRELLA = {1066, 1203, 197, 194};

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

const Sprite SPRITE_AI_UPHILL_LEFT_OPP_4 = {215, 1008, 80, 45};
const Sprite SPRITE_AI_UPHILL_STRAIGHT_OPP_4 = {127, 1065, 80, 45};
const Sprite SPRITE_AI_UPHILL_RIGHT_OPP_4 = {217, 1065, 80, 45};
const Sprite SPRITE_AI_LEFT_OPP_4 = {28, 954, 77, 41};
const Sprite SPRITE_AI_STRAIGHT_OPP_4 = {116, 954, 78, 41};
const Sprite SPRITE_AI_RIGHT_OPP_4 = {25, 1006, 77, 41};

const Sprite SPRITE_AI_UPHILL_LEFT_OPP_5 = {218, 1246, 80, 45};
const Sprite SPRITE_AI_UPHILL_STRAIGHT_OPP_5 = {130, 1303, 80, 45};
const Sprite SPRITE_AI_UPHILL_RIGHT_OPP_5 = {220, 1303, 80, 45};
const Sprite SPRITE_AI_LEFT_OPP_5 = {31, 1192, 77, 41};
const Sprite SPRITE_AI_STRAIGHT_OPP_5 = {119, 1192, 78, 41};
const Sprite SPRITE_AI_RIGHT_OPP_5 = {28, 1243, 77, 41};

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