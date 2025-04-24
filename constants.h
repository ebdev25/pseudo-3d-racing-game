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

typedef struct {
    const Sprite** frames;    // array of pointers to each frame's Sprite
    double* frameDurations;   // dynamically allocated array of durations
    int frameCount;           // how many frames
    double currentTime;       // how many seconds have elapsed for current frame
    int currentFrame;         // index of which frame is being displayed
    bool active;              // animation active flag
    bool finished;            // end of game flag
    bool loop;                // loop flag
} Animation;

typedef enum {
    COLLISION_NONE,
    COLLISION_FRONT,
    COLLISION_REAR,
    COLLISION_SIDE
} CollisionType;

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
extern const Color COLOR_FOG;

// External road color schemes
extern ColorScheme COLOR_LIGHT;
extern ColorScheme COLOR_DARK;
extern ColorScheme COLOR_START;
extern ColorScheme COLOR_FINISH;

// External sprite constants
extern const Sprite SPRITE_PALM_TREE_LEFT;
extern const Sprite SPRITE_PALM_TREE_RIGHT;
extern const Sprite SPRITE_ROCK;
extern const Sprite SPRITE_UMBRELLA;

extern const Sprite SPRITE_BILLBOARD08;
extern const Sprite SPRITE_BILLBOARD09;
extern const Sprite SPRITE_BILLBOARD01;
extern const Sprite SPRITE_BILLBOARD06;
extern const Sprite SPRITE_BILLBOARD05;
extern const Sprite SPRITE_BILLBOARD07;
extern const Sprite SPRITE_BILLBOARD04;
extern const Sprite SPRITE_BILLBOARD03;
extern const Sprite SPRITE_BILLBOARD02;

extern const Sprite SPRITE_PLAYER_UPHILL_LEFT;
extern const Sprite SPRITE_PLAYER_UPHILL_STRAIGHT;
extern const Sprite SPRITE_PLAYER_UPHILL_RIGHT;
extern const Sprite SPRITE_PLAYER_LEFT;
extern const Sprite SPRITE_PLAYER_STRAIGHT;
extern const Sprite SPRITE_PLAYER_RIGHT;

// Reference to player's straight sprite for scaling
#define PLAYER_SPRITE SPRITE_PLAYER_STRAIGHT

// AI Opponent sprite definitions
extern const Sprite SPRITE_AI_UPHILL_LEFT;
extern const Sprite SPRITE_AI_UPHILL_STRAIGHT;
extern const Sprite SPRITE_AI_UPHILL_RIGHT;
extern const Sprite SPRITE_AI_LEFT;
extern const Sprite SPRITE_AI_STRAIGHT;
extern const Sprite SPRITE_AI_RIGHT;

extern const Sprite SPRITE_AI_UPHILL_LEFT_OPP_2;
extern const Sprite SPRITE_AI_UPHILL_STRAIGHT_OPP_2;
extern const Sprite SPRITE_AI_UPHILL_RIGHT_OPP_2;
extern const Sprite SPRITE_AI_LEFT_OPP_2;
extern const Sprite SPRITE_AI_STRAIGHT_OPP_2;
extern const Sprite SPRITE_AI_RIGHT_OPP_2;

extern const Sprite SPRITE_AI_UPHILL_LEFT_OPP_3;
extern const Sprite SPRITE_AI_UPHILL_STRAIGHT_OPP_3;
extern const Sprite SPRITE_AI_UPHILL_RIGHT_OPP_3;
extern const Sprite SPRITE_AI_LEFT_OPP_3;
extern const Sprite SPRITE_AI_STRAIGHT_OPP_3;
extern const Sprite SPRITE_AI_RIGHT_OPP_3;

extern const Sprite SPRITE_AI_UPHILL_LEFT_OPP_4;
extern const Sprite SPRITE_AI_UPHILL_STRAIGHT_OPP_4;
extern const Sprite SPRITE_AI_UPHILL_RIGHT_OPP_4;
extern const Sprite SPRITE_AI_LEFT_OPP_4;
extern const Sprite SPRITE_AI_STRAIGHT_OPP_4;
extern const Sprite SPRITE_AI_RIGHT_OPP_4;

extern const Sprite SPRITE_AI_UPHILL_LEFT_OPP_5;
extern const Sprite SPRITE_AI_UPHILL_STRAIGHT_OPP_5;
extern const Sprite SPRITE_AI_UPHILL_RIGHT_OPP_5;
extern const Sprite SPRITE_AI_LEFT_OPP_5;
extern const Sprite SPRITE_AI_STRAIGHT_OPP_5;
extern const Sprite SPRITE_AI_RIGHT_OPP_5;

// Racing start lights
extern const Sprite SPRITE_LIGHT_FRAME1; 
extern const Sprite SPRITE_LIGHT_FRAME2; 
extern const Sprite SPRITE_LIGHT_FRAME3; 
extern const Sprite SPRITE_LIGHT_FRAME4; 
extern const Sprite SPRITE_LIGHT_FRAME5;

// Sprite arrays

#define BILLBOARD_COUNT 9
extern const Sprite* SPRITES_BILLBOARDS[BILLBOARD_COUNT];
Color hexToColor(const char* hexString);

#endif // CONSTANTS_H
