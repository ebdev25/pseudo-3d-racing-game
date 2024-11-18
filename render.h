// render.h
#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include "game.h"
#include "constants.h"

void render(Game* game);
void renderBackground(SDL_Renderer* renderer, SDL_Texture* background, int screenWidth, int screenHeight, 
                      const SDL_Rect* layerRect, double layerOffset, const Offset* layerPositionOffset, int offsetX);
void renderSegment(SDL_Renderer* renderer, int width, int lanes, double x1, double y1, double w1,
                   double x2, double y2, double w2, double fog, const ColorScheme* color);
void renderSprite(Game* game, SDL_Texture* spritesheet, const Sprite* sprite, double scale,
                  double destX, double destY, double offsetX, double offsetY, double clipY);
void renderPlayer(Game* game, double speedPercent, double scale, double destX, double destY,
                  double steer, double updown);
void renderPolygon(SDL_Renderer* renderer, double x1, double y1, double x2, double y2,
                   double x3, double y3, double x4, double y4, const Color* color);
void renderFog(SDL_Renderer* renderer, double x, double y, double width, double height, double fog);

double rumbleWidth(double projectedRoadWidth, int lanes);
double laneMarkerWidth(double projectedRoadWidth, int lanes);

#endif // RENDER_H