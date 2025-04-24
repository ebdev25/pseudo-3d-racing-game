// road.h
#ifndef ROAD_H
#define ROAD_H

#include "constants.h"
#include <SDL2/SDL.h>

struct Game;

typedef struct {
    const Sprite* source;
    double offset;
} SpriteInstance;

typedef struct {
    int index;
    double curve;
    Point p1, p2;
    ColorScheme color;
    SpriteInstance* sprites;
    int spriteCount;
    double clip;
} Segment;

typedef struct {
    Segment* segments;
    int segmentCount;
    double trackLength;
} Road;

void resetRoad(struct Game* game);
void resetSprites(struct Game* game);
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
Segment* findSegment(struct Game* game, double position);
double calculateElevation(Segment* segment, double position);

#endif // ROAD_H