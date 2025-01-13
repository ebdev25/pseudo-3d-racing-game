#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>
#include "color.h"

// Point structure for 3D-to-2D projections
typedef struct {
    struct {
        double x, y, z;
    } world;  // World coordinates
    struct {
        double x, y, w;
    } screen;  // Screen coordinates
    struct {
        double x, y, z;
    } camera;  // Camera-relative coordinates
    double scale;  // Perspective scaling factor
} Point;

// Road color scheme structure
typedef struct {
    Color road;
    Color grass;
    Color rumble;
    Color lane;
} ColorScheme;

// Sprite structure for texture regions
typedef struct {
    int x, y, w, h;
} Sprite;

// Game constants
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
#define MAX_CURVATURE 6.0


#define BACKGROUND_CYCLE_STATES_COUNT 3

#define GAP_SIZE 1200           // Gap between textures in pixels
#define SLIDE_SPEED 0.3f        // Speed of transition (progress per second)

// Blend factors for color saturation
#define MID_LOD_BLEND_FACTOR 0.2f   // 20% blend for mid LOD (L3)
#define HIGH_LOD_BLEND_FACTOR 0.3f  // 30% blend for high LOD (L4)

#define HILLS_PARALLAX_FACTOR 0.05
#define TREES_PARALLAX_FACTOR 0.10
#define MOUNTAINS_PARALLAX_FACTOR 0.12

#define MAX_SPEED (SEGMENT_LENGTH / STEP)
#define ACCELERATION (MAX_SPEED / 5)
#define BRAKING (-MAX_SPEED)
#define DECELERATION (-MAX_SPEED / 5)
#define OFF_ROAD_DECELERATION (-MAX_SPEED / 2)
#define OFF_ROAD_LIMIT (MAX_SPEED / 4)

#define SPRITE_SCALE (0.3 * (1.0 / PLAYER_SPRITE.w))

// Input key codes
#define KEY_LEFT SDLK_LEFT
#define KEY_UP SDLK_UP
#define KEY_RIGHT SDLK_RIGHT
#define KEY_DOWN SDLK_DOWN
#define KEY_A SDLK_a
#define KEY_D SDLK_d
#define KEY_S SDLK_s
#define KEY_W SDLK_w

// External color constants
extern const Color COLOR_SKY;
extern const Color COLOR_TREE;
extern const Color COLOR_FOG;

// External road color schemes
extern ColorScheme COLOR_LIGHT;
extern ColorScheme COLOR_DARK;
extern ColorScheme COLOR_START;
extern ColorScheme COLOR_FINISH;

// External sprite constants
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

// Reference to player's straight sprite for scaling
#define PLAYER_SPRITE SPRITE_PLAYER_STRAIGHT

// Sprite arrays
#define BILLBOARD_COUNT 9
extern const Sprite* SPRITES_BILLBOARDS[BILLBOARD_COUNT];

#define PLANT_COUNT 12
extern const Sprite* SPRITES_PLANTS[PLANT_COUNT];

#define CAR_COUNT 6
extern const Sprite* SPRITES_CARS[CAR_COUNT];

Color hexToColor(const char* hexString);

#endif // CONSTANTS_H
