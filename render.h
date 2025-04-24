#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include "constants.h"

struct Game;

// Function Prototypes
void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color, bool center);
void renderHUD(Game* game);
void render(Game* game);
void renderSlidingWindow(SDL_Renderer* renderer, const SlidingWindow* window, 
                        float cameraSpeed, int screenWidth, int screenHeight, bool disableWrapping);
void renderSegment(SDL_Renderer* renderer, int width, int lanes, double x1, double y1, double w1,
                   double x2, double y2, double w2, const ColorScheme* color);
void renderPolygon(SDL_Renderer* renderer, double x1, double y1, double x2, double y2,
                   double x3, double y3, double x4, double y4, const Color* color);
void renderSprite(Game* game, SDL_Texture* spritesheet, const Sprite* sprite, double scale,
                 double destX, double destY, double offsetX, double offsetY, double clipY);
void renderPlayer(Game* game, double speedPercent, double scale, double destX, double destY, double steer, double updown);
void renderAIOpponents(Game* game, Segment* segment);
double rumbleWidth(double projectedRoadWidth, int lanes);
double laneMarkerWidth(double projectedRoadWidth, int lanes);

#endif // RENDER_H
