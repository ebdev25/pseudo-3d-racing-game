// road.h
#ifndef ROAD_H
#define ROAD_H

#include "constants.h"
#include <SDL2/SDL.h>


// Forward declaration
struct Game;

typedef struct {
    const Sprite* source;
    double offset;
} SpriteInstance;

typedef struct {
    double offset;
    double z;
    const Sprite* sprite;
    double speed;
    double percent;
} Car;

typedef struct {
    int index;
    double curve;
    Point p1, p2;
    ColorScheme color;
    SpriteInstance* sprites;
    int spriteCount;
    Car** cars;
    int carCount;
    double clip;
} Segment;

typedef struct {
    Segment* segments;
    int segmentCount;
    double trackLength;
    Car* cars;
    int carCount;
} Road;

void buildRoad(struct Game* game);
Segment* findSegment(struct Game* game, double position);
void resetRoad(struct Game* game);
void resetSprites(struct Game* game);
void resetCars(struct Game* game);
double lastY(struct Game* game);
void addSegment(struct Game* game, double curve, double y);
void addSprite(struct Game* game, int segmentIndex, const Sprite* sprite, double offset);
void addRoad(struct Game* game, int enter, int hold, int leave, double curve, double y);
void addStraight(struct Game* game, int num);
void addHill(struct Game* game, int num, double height);
void addCurve(struct Game* game, int num, double curve, double height);
void addLowRollingHills(struct Game* game, int num, double height);
void addSCurves(struct Game* game);
void addBumps(struct Game* game);
void addDownhillToEnd(struct Game* game, int num);

#endif // ROAD_H