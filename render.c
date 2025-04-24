// render.c
#include "game.h"
#include "background.h"
#include "ai.h"
#include "util.h"
#include "constants.h"
#include "render.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static void renderStateLayers(SDL_Renderer* renderer,
                              const BackgroundCycleState* state,
                              int screenWidth,
                              int screenHeight,
                              bool disableWrapping)
{
    // For each of the 3 layers in this state
    for (int i = 0; i < 3; i++) {

        // Sanity check: if no texture, or invalid rect dims, skip
        if (!state->texture) {
            SDL_Log("renderStateLayers: No texture in state->texture for layer %d, skipping.\n", i);
            continue;
        }

        int texW = state->textureRects[i].w;
        int texH = state->textureRects[i].h;
        if (texW <= 0 || texH <= 0) {
            SDL_Log("renderStateLayers: Invalid texture dimensions (w=%d, h=%d) for layer %d, skipping.\n",
                    texW, texH, i);
            continue;
        }

        // Offsets for drawing this layer
        int offsetX = state->offsets[i].x;
        int offsetY = state->offsets[i].y;

        if (!disableWrapping)
        {
            //   NORMAL WRAPPING LOGIC
            int wrappedOffset = offsetX % texW;
            if (wrappedOffset < 0) {
                wrappedOffset += texW;
            }

            // Segment A width (first part of the texture)
            int segmentA_W = texW - wrappedOffset;
            // Segment B width (remaining portion)
            int segmentB_W = wrappedOffset;

            // Render Segment A at screen x=0
            SDL_Rect srcRectA = {
                state->textureRects[i].x + wrappedOffset,
                state->textureRects[i].y,
                segmentA_W,
                texH
            };
            SDL_Rect dstRectA = {
                0,
                offsetY,
                segmentA_W,
                texH
            };
            SDL_RenderCopy(renderer, state->texture, &srcRectA, &dstRectA);

            // If there's still screen space left, render Segment B
            int remaining = screenWidth - segmentA_W;
            if (remaining > 0) {
                int bW = (remaining < segmentB_W) ? remaining : segmentB_W;
                SDL_Rect srcRectB = {
                    state->textureRects[i].x,
                    state->textureRects[i].y,
                    bW,
                    texH
                };
                SDL_Rect dstRectB = {
                    segmentA_W,
                    offsetY,
                    bW,
                    texH
                };
                SDL_RenderCopy(renderer, state->texture, &srcRectB, &dstRectB);
            }
        }
        else
        {
            // NO WRAPPING => EXACT SINGLE COPY ON SCREEN

            bool twoSegmentsVisible = (state->extraSlice[i] > 0);

            if (!twoSegmentsVisible) {
                // CASE 1: Only One Segment is Visible
                SDL_Rect srcRect = {
                state->textureRects[i].x,
                state->textureRects[i].y,
                texW,
                texH
                };
                SDL_Rect dstRect = {
                    offsetX,
                    offsetY,
                    texW,
                    texH
                };

                // If the entire image is off to the left or right, skip
                int rightEdge = dstRect.x + dstRect.w;
                if (rightEdge <= 0 || dstRect.x >= screenWidth) {
                    // Entire layer is horizontally off-screen
                    continue;
                }

                // Clip if partially off the left side
                if (dstRect.x < 0) {
                    int offLeft = -dstRect.x;  // how many pixels are off-screen to the left
                    srcRect.x += offLeft;
                    srcRect.w -= offLeft;
                    dstRect.x = 0;
                    dstRect.w -= offLeft;
                }

                // Clip if partially off the right side
                rightEdge = dstRect.x + dstRect.w;
                if (rightEdge > screenWidth) {
                    int offRight = rightEdge - screenWidth;
                    srcRect.w -= offRight;
                    dstRect.w -= offRight;
                }

                // Now render if the rect is valid
                if (dstRect.w > 0 && dstRect.h > 0) {
                    SDL_RenderCopy(renderer, state->texture, &srcRect, &dstRect);
                }
            }
            else 
            {
                // CASE 2: Two Segments (A + B) are Visible

                int segmentA_Width = texW - abs(offsetX); // Remaining width of segment A
                int segmentB_Width = texW; // Full width of segment B

                // Segment A - Starts at its normal position but is cut off at the screen edge
                SDL_Rect srcRectA = {
                    state->textureRects[i].x + (texW - segmentA_Width), 
                    state->textureRects[i].y,
                    segmentA_Width,
                    texH
                };

                SDL_Rect dstRectA = {
                    0, // Always at the left edge of the screen
                    offsetY,
                    segmentA_Width,
                    texH
                };

                // Segment B - Should immediately follow segment A
                SDL_Rect srcRectB = {
                    state->textureRects[i].x, // Starts at the leftmost part of the texture
                    state->textureRects[i].y,
                    segmentB_Width,
                    texH
                };

                SDL_Rect dstRectB = {
                    segmentA_Width, // Starts exactly where segment A ends
                    offsetY,
                    segmentB_Width,
                    texH
                };

                // Render both segments
                SDL_RenderCopy(renderer, state->texture, &srcRectA, &dstRectA);
                SDL_RenderCopy(renderer, state->texture, &srcRectB, &dstRectB);
            }
        }
    }
}

static const Sprite* pickAISprite(AIDriver* driver, double updown) {
    double steer = driver->offset - driver->targetLaneOffset;
    int variant = driver->variant;  // Get the assigned variant

    if (variant == 1) {
        if (steer < -0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_LEFT : &SPRITE_AI_LEFT;
        } else if (steer > 0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_RIGHT : &SPRITE_AI_RIGHT;
        } else {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_STRAIGHT : &SPRITE_AI_STRAIGHT;
        }
    } else if (variant == 2) {
        if (steer < -0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_LEFT_OPP_2 : &SPRITE_AI_LEFT_OPP_2;
        } else if (steer > 0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_RIGHT_OPP_2 : &SPRITE_AI_RIGHT_OPP_2;
        } else {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_STRAIGHT_OPP_2 : &SPRITE_AI_STRAIGHT_OPP_2;
        }
    } else if (variant == 3) {
        if (steer < -0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_LEFT_OPP_3 : &SPRITE_AI_LEFT_OPP_3;
        } else if (steer > 0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_RIGHT_OPP_3 : &SPRITE_AI_RIGHT_OPP_3;
        } else {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_STRAIGHT_OPP_3 : &SPRITE_AI_STRAIGHT_OPP_3;
        }
    } else if (variant == 4) {
        if (steer < -0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_LEFT_OPP_4 : &SPRITE_AI_LEFT_OPP_4;
        } else if (steer > 0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_RIGHT_OPP_4 : &SPRITE_AI_RIGHT_OPP_4;
        } else {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_STRAIGHT_OPP_4 : &SPRITE_AI_STRAIGHT_OPP_4;
        }
    } else if (variant == 5) {
        if (steer < -0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_LEFT_OPP_5 : &SPRITE_AI_LEFT_OPP_5;
        } else if (steer > 0.01) {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_RIGHT_OPP_5 : &SPRITE_AI_RIGHT_OPP_5;
        } else {
            return (updown > 0.0) ? &SPRITE_AI_UPHILL_STRAIGHT_OPP_5 : &SPRITE_AI_STRAIGHT_OPP_5;
        }
    }
    // Default to variant 1 if something goes wrong
    if (steer < -0.01) {
        return (updown > 0.0) ? &SPRITE_AI_UPHILL_LEFT : &SPRITE_AI_LEFT;
    } else if (steer > 0.01) {
        return (updown > 0.0) ? &SPRITE_AI_UPHILL_RIGHT : &SPRITE_AI_RIGHT;
    } else {
        return (updown > 0.0) ? &SPRITE_AI_UPHILL_STRAIGHT : &SPRITE_AI_STRAIGHT;
    }
}

static void renderTrafficLight(Game* game)
{
    Animation* anim = &game->trafficLightAnim;
    if (!anim->active) {
        return; // no lights to show
    }

    int trafficLightSegmentIndex = 16; // choose a segment near the start line
    if (trafficLightSegmentIndex < 0 || 
        trafficLightSegmentIndex >= game->road.segmentCount)
        return;

    Segment* seg = &game->road.segments[trafficLightSegmentIndex];

    // sprite scale and position for projection
    double spriteScale = seg->p1.scale;  
    double spriteX = seg->p1.screen.x + (spriteScale * (-1.2) * 
                        ROAD_WIDTH * game->width / 2.0);
    double spriteY = seg->p1.screen.y;

    // Current frame in the animation
    const Sprite* frameSprite = anim->frames[anim->currentFrame];

    renderSprite(game, 
                 game->animatedSpritesheet, 
                 frameSprite,
                 spriteScale,
                 spriteX,
                 spriteY,
                 -0.5, // x offset 
                 -1.0, // y offset (draw from bottom)
                 seg->clip);
}

// Helper function to render text (used by HUD and Leaderboard message)
void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text,
                int x, int y, SDL_Color color, bool center)
{
    if (!font || !text) {
        SDL_Log("renderText: Font or text is NULL.");
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) {
        SDL_Log("TTF_RenderText_Blended Error: %s", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dstRect;
    dstRect.y = y;
    dstRect.w = surface->w;
    dstRect.h = surface->h;

    if (center) {
        dstRect.x = x - surface->w / 2; // Center horizontally around x
    } else {
        dstRect.x = x; // Align left edge at x
    }

    SDL_RenderCopy(renderer, texture, NULL, &dstRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Function to render the Heads-Up Display (HUD)
void renderHUD(Game* game) {
    if (!game->hudFont) return; // Need font to render HUD

    SDL_Color textColor = {255, 255, 255, 255}; // White text
    char buffer[64];

    // Render Speed
    int displaySpeed = (int)(game->speed / 100.0);
    snprintf(buffer, sizeof(buffer), "SPD: %d", displaySpeed);
    renderText(game->renderer, game->hudFont, buffer, 20, 20, textColor, false);

    // Render Current Lap Time
    snprintf(buffer, sizeof(buffer), "Lap: %.2f", game->currentLapTime);
    // Need to measure text width to right-align properly
    int textW, textH;
    TTF_SizeText(game->hudFont, buffer, &textW, &textH);
    renderText(game->renderer, game->hudFont, buffer, game->width - textW - 20, 20, textColor, false);

    // Render Lap Count
    snprintf(buffer, sizeof(buffer), "Lap %d / %d", game->playerLapsCompleted + 1, game->totalLaps);
    TTF_SizeText(game->hudFont, buffer, &textW, &textH);
    renderText(game->renderer, game->hudFont, buffer, game->width - textW - 20, game->height - textH - 20, textColor, false);

    // 4. Render Last Lap Time
    if (game->lastLapTime > 0.0) {
        snprintf(buffer, sizeof(buffer), "Last: %.2f", game->lastLapTime);
        TTF_SizeText(game->hudFont, buffer, &textW, &textH);
        renderText(game->renderer, game->hudFont, buffer, game->width - textW - 20, 20 + textH + 5, textColor, false);
    }
}

// Main Render Function
void render(Game* game) {
    SDL_Renderer* renderer = game->renderer;
    int width = game->width;
    int height = game->height;
    double resolution = game->resolution;
    double roadWidth = ROAD_WIDTH;
    double cameraHeight = CAMERA_HEIGHT;
    double cameraDepth = game->cameraDepth;
    double playerZ = game->playerZ;
    double position = game->position;
    double playerX = game->playerX;
    double speed = game->speed;
    double maxSpeed = game->maxSpeed;
    int lanes = LANE_COUNT;
    int drawDistance = DRAW_DISTANCE;

    SDL_Texture* background = game->background;
    SDL_Texture* spritesheet = game->spritesheet;

    // Clear the screen with black to reset the frame
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int screenWidth = game->width;
    int screenHeight = game->height;

    // Background rendering logic
    float cameraSpeed = (float)(speed / maxSpeed); // Speed factor for parallax scrolling
    renderSlidingWindow(renderer, game->slidingWindow, cameraSpeed, screenWidth, screenHeight, game->transitionHappening);

    // Initialize player Y-coordinate to 0; will be calculated based on segment position
    double playerY = 0.0;

    // Find the base segment (where the player currently is) and the interpolation value within the segment
    Segment* baseSegment = findSegment(game, position);
    double basePercent = percentRemaining(position, SEGMENT_LENGTH);

    // Locate the segment where the player will be positioned (slightly ahead) and the interpolation within it
    Segment* playerSegment = findSegment(game, position + playerZ);
    double playerPercent = percentRemaining(position + playerZ, SEGMENT_LENGTH);

    // Interpolate the player's Y-position based on the segment data
    playerY = interpolate(playerSegment->p1.world.y, playerSegment->p2.world.y, playerPercent);

    // Set the maximum Y value to the screen height for clipping purposes
    double maxY = (double)height;

    // Initialize X position and delta X for curve effect in the road
    double x = 0.0;
    double dx = - (baseSegment->curve * basePercent);

    double highestRoadY = (double)game->height; // Since Y=0 is the top, start high
    double lowestRoadY = 0.0;                   // Start low (0)
    // Loop through each segment in the visible range
    for (int n = 0; n < drawDistance; n++) {
        // Calculate the current segment index, accounting for looping if beyond track end
        int currentIndex = (baseSegment->index + n) % game->road.segmentCount;
        Segment* segment = &game->road.segments[currentIndex];
        int looped = (segment->index < baseSegment->index) ? 1 : 0;

        // Set clipping limit to the current max Y
        segment->clip = maxY;

        // Calculate camera-relative coordinates for projecting segment points
        double cameraX = playerX * roadWidth - x;
        double cameraY = playerY + cameraHeight;
        double cameraZ = position - (looped ? game->road.trackLength : 0.0);

        // Project the segment's start and end points based on camera position and depth
        project(&segment->p1, cameraX, cameraY, cameraZ, cameraDepth, width, height, roadWidth);
        project(&segment->p2, cameraX - dx, cameraY, cameraZ, cameraDepth, width, height, roadWidth);

        // Update X and delta X based on segment curve to adjust future segments
        x += dx;
        dx += segment->curve;

        // Skip rendering if the segment is behind the camera or clipped offscreen
        if (segment->p1.camera.z <= cameraDepth ||
            segment->p2.screen.y >= segment->p1.screen.y ||
            segment->p2.screen.y >= maxY)
            continue;

        // Render the road segment based on screen coordinates
        renderSegment(renderer, width, lanes,
                      segment->p1.screen.x,
                      segment->p1.screen.y,
                      segment->p1.screen.w,
                      segment->p2.screen.x,
                      segment->p2.screen.y,
                      segment->p2.screen.w,
                      &segment->color);

            // Update highestRoadY and lowestRoadY:
        if (segment->p1.screen.y < highestRoadY) {
            highestRoadY = segment->p1.screen.y;
        }
        if (segment->p2.screen.y > lowestRoadY) {
            lowestRoadY = segment->p2.screen.y;
        }

        game->highestRoadY = highestRoadY;
        game->lowestRoadY = lowestRoadY;

        // Update max Y to the top of this segment for subsequent clipping
        maxY = segment->p1.screen.y;
    }

    // Render roadside sprites and cars from farthest to nearest segments
    for (int n = drawDistance - 1; n > 0; n--) {
        int currentIndex = (baseSegment->index + n) % game->road.segmentCount;
        Segment* segment = &game->road.segments[currentIndex];

        // Render each roadside sprite (e.g., trees, billboards) in the segment
        for (int i = 0; i < segment->spriteCount; i++) {
            SpriteInstance* spriteInstance = &segment->sprites[i];
            const Sprite* sprite = spriteInstance->source;
            double spriteScale = segment->p1.scale;
            double spriteX = segment->p1.screen.x +
                             (spriteScale * spriteInstance->offset * roadWidth * width / 2.0);
            double spriteY = segment->p1.screen.y;

            // Render the roadside sprite with calculated properties
            renderSprite(game, spritesheet, sprite, spriteScale, spriteX, spriteY,
                        (spriteInstance->offset < 0.0 ? -1.0 : 0.0), -1.0, segment->clip);
        }

        renderAIOpponents(game, segment);

        // Render the player sprite in the current segment if it's the player segment
        if (segment == playerSegment) {
            double speedPercent = (speed / maxSpeed);
            double playerScale = interpolate(playerSegment->p1.scale, playerSegment->p2.scale, playerPercent);
            double playerDestX = (double)width / 2.0;

            // Calculate the vertical position for player sprite with slight offset
            double playerDestY = ((double)height / 2.0) - 
                                 (playerScale * interpolate(playerSegment->p1.camera.y, 
                                                            playerSegment->p2.camera.y, 
                                                            playerPercent) * (double)height / 2.0) - 150.0;

            // Determine steering and elevation change for rendering player
            double steer = (game->keyLeft ? -1.0 : (game->keyRight ? 1.0 : 0.0));
            double updown = playerSegment->p2.world.y - playerSegment->p1.world.y;

            // Render the player car
            renderPlayer(game, speedPercent, playerScale, playerDestX, playerDestY, steer, updown);
        }

        renderTrafficLight(game);
    }
}

void renderSlidingWindow(SDL_Renderer* renderer,
                         const SlidingWindow* window,
                         float cameraSpeed,
                         int screenWidth,
                         int screenHeight,
                         bool disableWrapping)
{
    if (!window || window->cycleStateCount <= 0) 
        return;

    // Grab the current cycle state
    const BackgroundCycleState* current =
        &window->cycleStates[window->currentCycleStateIndex];

    // Fill the sky color from the current state
    SDL_SetRenderDrawColor(renderer,
                           window->interpolatedSkyColor.r,
                           window->interpolatedSkyColor.g,
                           window->interpolatedSkyColor.b,
                           window->interpolatedSkyColor.a);
    SDL_Rect skyRect = {0, 0, screenWidth, screenHeight};
    SDL_RenderFillRect(renderer, &skyRect);

    // Render the layers of the current state
    renderStateLayers(renderer, current, screenWidth, screenHeight, disableWrapping);

    // If transition happening, also render the target state's layers
    if (window->transitioning &&
        window->targetCycleStateIndex >= 0 &&
        window->targetCycleStateIndex < window->cycleStateCount)
    {
        const BackgroundCycleState* target =
            &window->cycleStates[window->targetCycleStateIndex];

        renderStateLayers(renderer, target, screenWidth, screenHeight, disableWrapping);
    }
}

// Render a road segment with rumble strips, road surface, and lane markers
void renderSegment(SDL_Renderer* renderer, int width, int lanes, double x1, double y1, double w1,
                   double x2, double y2, double w2, const ColorScheme* color) {
    // Calculate rumble strip width based on lane width for the top and bottom of the segment
    double r1 = rumbleWidth(w1, lanes);
    double r2 = rumbleWidth(w2, lanes);

    // Calculate lane marker width for the top and bottom of the segment
    double l1 = laneMarkerWidth(w1, lanes);
    double l2 = laneMarkerWidth(w2, lanes);

    double lanew1, lanew2, lanex1, lanex2;

    // Ensure y1 is greater than y2 to render the grass layer correctly
    if (y1 > y2) {
        int grassHeight = (int)(y1 - y2);  // Calculate height of the grass area
        if (grassHeight > 0) {
            // Set grass color and render grass rectangle across the width of the screen
            SDL_SetRenderDrawColor(renderer, color->grass.r, color->grass.g, color->grass.b, color->grass.a);
            SDL_Rect grassRect = {0, (int)y2, width, grassHeight};
            SDL_RenderFillRect(renderer, &grassRect);
        }
    }

    // Draw left-side rumble strip with specified color
    renderPolygon(renderer,
                  x1 - w1 - r1, y1,
                  x1 - w1, y1,
                  x2 - w2, y2,
                  x2 - w2 - r2, y2,
                  &color->rumble);

    // Draw right-side rumble strip
    renderPolygon(renderer,
                  x1 + w1 + r1, y1,
                  x1 + w1, y1,
                  x2 + w2, y2,
                  x2 + w2 + r2, y2,
                  &color->rumble);

    // Draw the road surface between rumble strips
    renderPolygon(renderer,
                  x1 - w1, y1,
                  x1 + w1, y1,
                  x2 + w2, y2,
                  x2 - w2, y2,
                  &color->road);

    // If lane markers are visible, draw them between lanes
    if (color->lane.a != 0) {
        lanew1 = w1 * 2.0 / lanes;  // Width of each lane at the top of the segment
        lanew2 = w2 * 2.0 / lanes;  // Width of each lane at the bottom of the segment
        lanex1 = x1 - w1 + lanew1;  // Starting x position for lane marker
        lanex2 = x2 - w2 + lanew2;

        // Draw each lane marker as a narrow polygon strip
        for (int lane = 1; lane < lanes; lane++, lanex1 += lanew1, lanex2 += lanew2) {
            renderPolygon(renderer,
                          lanex1 - l1 / 2.0, y1,
                          lanex1 + l1 / 2.0, y1,
                          lanex2 + l2 / 2.0, y2,
                          lanex2 - l2 / 2.0, y2,
                          &color->lane);
        }
    }
}

// Render a filled polygon using SDL_RenderGeometry
void renderPolygon(SDL_Renderer* renderer, double x1, double y1, double x2, double y2,
                   double x3, double y3, double x4, double y4, const Color* color) {
    // Define an array of 4 vertices representing the corners of the polygon
    SDL_Vertex verts[4] = {
        { { (float)x1, (float)y1 }, { color->r, color->g, color->b, color->a }, { 0.0f, 0.0f } },
        { { (float)x2, (float)y2 }, { color->r, color->g, color->b, color->a }, { 0.0f, 0.0f } },
        { { (float)x3, (float)y3 }, { color->r, color->g, color->b, color->a }, { 0.0f, 0.0f } },
        { { (float)x4, (float)y4 }, { color->r, color->g, color->b, color->a }, { 0.0f, 0.0f } }
    };

    // Define the order of indices to draw two triangles for a quad
    int indices[6] = { 0, 1, 2, 2, 3, 0 };

    // Render the polygon as a quad using SDL_RenderGeometry
    if (SDL_RenderGeometry(renderer, NULL, verts, 4, indices, 6) != 0) {
        printf("SDL_RenderGeometry error: %s\n", SDL_GetError());
    }
}

// Render a sprite with scaling, positioning, and clipping
void renderSprite(Game* game, SDL_Texture* spritesheet, const Sprite* sprite, double scale,
                 double destX, double destY, double offsetX, double offsetY, double clipY) {
    if (!sprite) {
        SDL_Log("renderSprite: sprite is NULL, skipping rendering.");
        return;
    }

    SDL_Renderer* renderer = game->renderer;
    int width = game->width;
    int height = game->height;

    // Dimensions of the original sprite
    int spriteWidth = sprite->w;
    int spriteHeight = sprite->h;

    // Calculate scaled width and height for the sprite based on road perspective and scale factors
    double destW = ((double)spriteWidth * scale * SPRITE_SCALE * (double)width / 2.0) * ROAD_WIDTH;
    double destH = ((double)spriteHeight * scale * SPRITE_SCALE * (double)width / 2.0) * ROAD_WIDTH;

    // Adjust the destination X and Y based on offset values
    destX += destW * (offsetX != 0.0 ? offsetX : 0.0);
    destY += destH * (offsetY != 0.0 ? offsetY : 0.0);

    // Calculate clipping for the Y-axis to avoid rendering parts outside viewable area
    double clipH = (clipY > 0.0) ? fmax(0.0, destY + destH - clipY) : 0.0;
    if (clipH < destH) {
        // Define the source rectangle on the spritesheet for the unclipped portion
        SDL_Rect srcRect = { sprite->x, sprite->y, sprite->w, (int)(sprite->h - (sprite->h * clipH / destH)) };

        // Define the destination rectangle on the screen, applying scaling and clipping
        SDL_Rect destRect = { (int)destX, (int)destY, (int)destW, (int)(destH - clipH) };

        // Copy the portion of the sprite from the spritesheet to the screen
        SDL_RenderCopy(renderer, spritesheet, &srcRect, &destRect);
    }
}

// Render the player car with position and bounce adjustments
void renderPlayer(Game* game, double speedPercent, double scale, double destX, double destY, double steer, double updown) {
    SDL_Texture* spritesheet = game->spritesheet;

    // Calculate a "bounce" effect based on speed and a random factor
    double bounce = (1.5 * ((double)rand() / RAND_MAX) * speedPercent * game->resolution) *
                    (((rand() % 2) == 0) ? -1.0 : 1.0);

    const Sprite* sprite;

    // Determine which sprite to use based on steering and elevation change
    if (steer < 0.0)
        sprite = (updown > 0.0) ? &SPRITE_PLAYER_UPHILL_LEFT : &SPRITE_PLAYER_LEFT;
    else if (steer > 0.0)
        sprite = (updown > 0.0) ? &SPRITE_PLAYER_UPHILL_RIGHT : &SPRITE_PLAYER_RIGHT;
    else
        sprite = (updown > 0.0) ? &SPRITE_PLAYER_UPHILL_STRAIGHT : &SPRITE_PLAYER_STRAIGHT;

    // Render the player sprite with position and bounce adjustments
    game->playerRenderedY = destY + bounce;
    renderSprite(game, spritesheet, sprite, scale, destX, destY + bounce, -0.5, -1.0, 0.0);
}

void renderAIOpponents(Game* game, Segment* segment)
{
    AIController* ctrl = &game->aiController;
    SDL_Texture* aiTex = game->opponentSpritesheet;

    // Loop over all AI drivers
    for (int i = 0; i < ctrl->driverCount; i++) {
        AIDriver* driver = &ctrl->drivers[i];

        // Find which segment this driver is currently on
        Segment* seg = findSegment(game, driver->z);
        if (!seg) continue;

        // If it's not the same segment currently rendering, skip it
        if (seg != segment) {
            continue;
        }

        // else continue normal rendering

        double spriteScale = interpolate(seg->p1.scale, seg->p2.scale, driver->percent);
        spriteScale *= 1.25;
        double spriteX = interpolate(seg->p1.screen.x, seg->p2.screen.x, driver->percent)
                       + (spriteScale * driver->offset * ROAD_WIDTH * game->width / 2.0);

        double spriteY = interpolate(seg->p1.screen.y, seg->p2.screen.y, driver->percent);

        double updown = seg->p2.world.y - seg->p1.world.y;
        const Sprite* sprite = pickAISprite(driver, updown);

        // Finally, render
        driver->renderedY = spriteY;
        renderSprite(game, aiTex, sprite, spriteScale, spriteX, spriteY, -0.5, -1.0, seg->clip);
    }
}

// Calculate the width of the rumble strip based on the projected road width and lane count
double rumbleWidth(double projectedRoadWidth, int lanes) {
    return projectedRoadWidth / fmax(6.0, 2.0 * (double)lanes);
}

// Calculate the width of the lane markers based on the projected road width and lane count
double laneMarkerWidth(double projectedRoadWidth, int lanes) {
    return projectedRoadWidth / fmax(32.0, 8.0 * (double)lanes);
}