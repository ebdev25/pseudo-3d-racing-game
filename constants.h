// constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SDL2/SDL.h>
#include <math.h>

// Define Point
typedef struct {
    struct {
        double x, y, z;
    } world;
    struct {
        double x, y, w;
    } screen;
    struct {
        double x, y, z;
    } camera;
    double scale;
} Point;

// Color structure
typedef struct {
    Uint8 r, g, b, a;
} Color;

// Road color scheme structure
typedef struct {
    Color road;
    Color grass;
    Color rumble;
    Color lane;
} ColorScheme;

// Sprite structure
typedef struct {
    int x, y, w, h;
} Sprite;

// Offset structure
typedef struct {
    int x,y;
} Offset;

// LOD region structure
typedef struct {
    int x, y, w, h;
} LODRegion;

typedef struct {
    SDL_Rect highLOD;
    SDL_Rect midLOD;
    SDL_Rect lowLOD;
} BackgroundLayer;

typedef struct {
    const SDL_Rect* L2; // Low LOD
    const Offset* L2Offset; // Unique offst for the texture in Layer 2
    const SDL_Rect* L3; // Mid LOD
    const Offset* L3Offset; // Unique offst for the texture in Layer 3
    const SDL_Rect* L4; // High LOD
    const Offset* L4Offset; // Unique offst for the texture in Layer 4
} BackgroundCycleState;


#define FPS 60
#define STEP (1.0 / FPS)
#define WIDTH 1024
#define HEIGHT 768
#define ROAD_WIDTH 2000
#define SEGMENT_LENGTH 200
#define RUMBLE_LENGTH 3
#define LANE_COUNT 3
#define FIELD_OF_VIEW 100
#define CAMERA_HEIGHT 1000
#define DRAW_DISTANCE 300
#define FOG_DENSITY 5
//#define M_PI 3.14159265359

#define MAX_SPEED (SEGMENT_LENGTH / STEP)
#define ACCELERATION (MAX_SPEED / 5)
#define BRAKING (-MAX_SPEED)
#define DECELERATION (-MAX_SPEED / 5)
#define OFF_ROAD_DECELERATION (-MAX_SPEED / 2)
#define OFF_ROAD_LIMIT (MAX_SPEED / 4)

#define SPRITE_SCALE (0.3 * (1.0 / PLAYER_SPRITE.w))

// Key codes
#define KEY_LEFT  SDLK_LEFT
#define KEY_UP    SDLK_UP
#define KEY_RIGHT SDLK_RIGHT
#define KEY_DOWN  SDLK_DOWN
#define KEY_A     SDLK_a
#define KEY_D     SDLK_d
#define KEY_S     SDLK_s
#define KEY_W     SDLK_w

// Color constants
extern const Color COLOR_SKY;
extern const Color COLOR_TREE;
extern const Color COLOR_FOG;

// Color schemes
extern const ColorScheme COLOR_LIGHT;
extern const ColorScheme COLOR_DARK;
extern const ColorScheme COLOR_START;
extern const ColorScheme COLOR_FINISH;

// Background layers
extern const BackgroundLayer BACKGROUND_HILLS;
extern const BackgroundLayer BACKGROUND_SKY;
extern const BackgroundLayer BACKGROUND_TREES;
extern const BackgroundLayer BACKGROUND_HOUSES;

// Cycle states for LOD cycling
extern const BackgroundCycleState BACKGROUND_CYCLE_STATES[3];

// Sprite constants
extern const Sprite SPRITE_PALM_TREE;
extern const Sprite SPRITE_BILLBOARD08;
extern const Sprite SPRITE_TREE1;
extern const Sprite SPRITE_DEAD_TREE1;
extern const Sprite SPRITE_BILLBOARD09;
extern const Sprite SPRITE_BOULDER3;
extern const Sprite SPRITE_COLUMN;
extern const Sprite SPRITE_BILLBOARD01;
extern const Sprite SPRITE_BILLBOARD06;
extern const Sprite SPRITE_BILLBOARD05;
extern const Sprite SPRITE_BILLBOARD07;
extern const Sprite SPRITE_BOULDER2;
extern const Sprite SPRITE_TREE2;
extern const Sprite SPRITE_BILLBOARD04;
extern const Sprite SPRITE_DEAD_TREE2;
extern const Sprite SPRITE_BOULDER1;
extern const Sprite SPRITE_BUSH1;
extern const Sprite SPRITE_CACTUS;
extern const Sprite SPRITE_BUSH2;
extern const Sprite SPRITE_BILLBOARD03;
extern const Sprite SPRITE_BILLBOARD02;
extern const Sprite SPRITE_STUMP;
extern const Sprite SPRITE_SEMI;
extern const Sprite SPRITE_TRUCK;
extern const Sprite SPRITE_CAR03;
extern const Sprite SPRITE_CAR02;
extern const Sprite SPRITE_CAR04;
extern const Sprite SPRITE_CAR01;
extern const Sprite SPRITE_PLAYER_UPHILL_LEFT;
extern const Sprite SPRITE_PLAYER_UPHILL_STRAIGHT;
extern const Sprite SPRITE_PLAYER_UPHILL_RIGHT;
extern const Sprite SPRITE_PLAYER_LEFT;
extern const Sprite SPRITE_PLAYER_STRAIGHT;
extern const Sprite SPRITE_PLAYER_RIGHT;

// Reference to the player's straight sprite for SPRITE_SCALE calculation
#define PLAYER_SPRITE SPRITE_PLAYER_STRAIGHT

// Arrays of sprites
#define BILLBOARD_COUNT 9
extern const Sprite* SPRITES_BILLBOARDS[BILLBOARD_COUNT];

#define PLANT_COUNT 12
extern const Sprite* SPRITES_PLANTS[PLANT_COUNT];

#define CAR_COUNT 6
extern const Sprite* SPRITES_CARS[CAR_COUNT];

#endif // CONSTANTS_H