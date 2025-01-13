// render.c
#include "game.h"
#include "background.h"
#include "util.h"
#include "constants.h"
#include "render.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

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
    double fogDensity = FOG_DENSITY;

    SDL_Texture* background = game->background;
    SDL_Texture* spritesheet = game->spritesheet;

    // Clear the screen with black to reset the frame
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int screenWidth = game->width;
    int screenHeight = game->height;

    // Background rendering logic
    float cameraSpeed = (float)(speed / maxSpeed); // Speed factor for parallax scrolling
    renderSlidingWindow(renderer, game->slidingWindow, cameraSpeed, screenWidth, screenHeight);

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

    // Loop through each segment in the visible range
    for (int n = 0; n < drawDistance; n++) {
        // Calculate the current segment index, accounting for looping if beyond track end
        int currentIndex = (baseSegment->index + n) % game->road.segmentCount;
        Segment* segment = &game->road.segments[currentIndex];
        int looped = (segment->index < baseSegment->index) ? 1 : 0;

        // Apply exponential fog to create a depth effect as distance increases
        double fog = exponentialFog((double)n / drawDistance, fogDensity);

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

        // Render the road segment based on screen coordinates and apply fog color
        renderSegment(renderer, width, lanes,
                      segment->p1.screen.x,
                      segment->p1.screen.y,
                      segment->p1.screen.w,
                      segment->p2.screen.x,
                      segment->p2.screen.y,
                      segment->p2.screen.w,
                      fog,
                      &segment->color);

        // Update max Y to the top of this segment for subsequent clipping
        maxY = segment->p1.screen.y;
    }

    // Render roadside sprites and cars from farthest to nearest segments
    for (int n = drawDistance - 1; n > 0; n--) {
        int currentIndex = (baseSegment->index + n) % game->road.segmentCount;
        Segment* segment = &game->road.segments[currentIndex];

        // Render each car in the segment
        for (int i = 0; i < segment->carCount; i++) {
            Car* car = segment->cars[i];
            const Sprite* sprite = car->sprite;

            // Interpolate scale and screen position based on segment's points
            double spriteScale = interpolate(segment->p1.scale, segment->p2.scale, car->percent);
            double spriteX = interpolate(segment->p1.screen.x, segment->p2.screen.x, car->percent) +
                             (spriteScale * car->offset * roadWidth * width / 2.0);
            double spriteY = interpolate(segment->p1.screen.y, segment->p2.screen.y, car->percent);

            // Render the car sprite with calculated properties
            renderSprite(game, spritesheet, sprite, spriteScale, spriteX, spriteY, -0.5, -1.0, segment->clip);
        }

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

        // Render the player sprite in the current segment if it's the player segment
        if (segment == playerSegment) {
            double speedPercent = (speed / maxSpeed);
            double playerScale = cameraDepth / playerZ;
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
    }
}

void renderSlidingWindow(SDL_Renderer* renderer, const SlidingWindow* window, float cameraSpeed, 
                         int screenWidth, int screenHeight) {
    const BackgroundCycleState* current = &window->cycleStates[window->currentCycleStateIndex];
    const BackgroundCycleState* target = (window->transitioning) ? &window->cycleStates[window->targetCycleStateIndex] : NULL;

    // Render sky background
    SDL_SetRenderDrawColor(renderer, 
                           window->interpolatedSkyColor.r, 
                           window->interpolatedSkyColor.g, 
                           window->interpolatedSkyColor.b, 
                           window->interpolatedSkyColor.a);
    SDL_Rect skyRect = { 0, 0, screenWidth, screenHeight };
    SDL_RenderFillRect(renderer, &skyRect);

    // Render parallax layers
    for (int i = 0; i < 3; i++) {
        // Calculate horizontal offset with slideOffsetX
        int offsetX = current->offsets[i].x + (int)(current->offsets[i].slideOffsetX) 
                      + (int)(cameraSpeed * current->speeds[i]);
        offsetX = offsetX % current->textureRects[i].w; // Wrap horizontally
        if (offsetX < 0) offsetX += current->textureRects[i].w;

        // Calculate vertical offset
        int offsetY = current->offsets[i].y + (int)(cameraSpeed * current->verticalSpeeds[i]);

        SDL_Rect destRect = { offsetX, offsetY, current->textureRects[i].w, current->textureRects[i].h };
        SDL_Rect wrappedDestRect = destRect;
        wrappedDestRect.x -= current->textureRects[i].w; // Horizontal wrapping

        if (window->transitioning && target) {
            // Compute eased transition progress
            float easedProgress = easeInOut(0.0, 1.0, window->transitionProgress);

            // Calculate alpha blending for current and target textures
            Uint8 currentAlpha = (Uint8)((1.0f - easedProgress) * 255);
            Uint8 targetAlpha = (Uint8)(easedProgress * 255);

            // Apply slideOffsetX to target layer
            int targetOffsetX = target->offsets[i].x + (int)(target->offsets[i].slideOffsetX) 
                                + (int)(cameraSpeed * target->speeds[i]);
            targetOffsetX = targetOffsetX % target->textureRects[i].w; // Wrap horizontally
            if (targetOffsetX < 0) targetOffsetX += target->textureRects[i].w;

            int targetOffsetY = target->offsets[i].y + (int)(cameraSpeed * target->verticalSpeeds[i]);

            SDL_Rect targetDestRect = { targetOffsetX, targetOffsetY, target->textureRects[i].w, target->textureRects[i].h };
            SDL_Rect targetWrappedDestRect = targetDestRect;
            targetWrappedDestRect.x -= target->textureRects[i].w; // Horizontal wrapping

            // Render current layer with alpha blending
            SDL_SetTextureAlphaMod(current->texture, currentAlpha);
            SDL_RenderCopy(renderer, current->texture, &current->textureRects[i], &destRect);
            SDL_RenderCopy(renderer, current->texture, &current->textureRects[i], &wrappedDestRect);

            // Render target layer with alpha blending
            SDL_SetTextureAlphaMod(target->texture, targetAlpha);
            SDL_RenderCopy(renderer, target->texture, &target->textureRects[i], &targetDestRect);
            SDL_RenderCopy(renderer, target->texture, &target->textureRects[i], &targetWrappedDestRect);

            // Reset alpha modulation
            SDL_SetTextureAlphaMod(current->texture, 255);
            if (current->texture != target->texture) {
                SDL_SetTextureAlphaMod(target->texture, 255);
            }
        } else {
            // Render current layer only (no transition)
            SDL_RenderCopy(renderer, current->texture, &current->textureRects[i], &destRect);
            SDL_RenderCopy(renderer, current->texture, &current->textureRects[i], &wrappedDestRect);
        }
    }
}

// Render a road segment with rumble strips, road surface, and lane markers
void renderSegment(SDL_Renderer* renderer, int width, int lanes, double x1, double y1, double w1,
                   double x2, double y2, double w2, double fog, const ColorScheme* color) {
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

    // Apply fog effect from the top to the bottom of the segment (if needed)
    // renderFog(renderer, 0, y1, width, y2 - y1, fog); // Uncomment if fog is to be rendered here
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
    renderSprite(game, spritesheet, sprite, scale, destX, destY + bounce, -0.5, -1.0, 0.0);
}

// Render a fog overlay (if needed)
void renderFog(SDL_Renderer* renderer, double x, double y, double width, double height, double fog) {
    // Only render fog if the fog density is below 1 (visible fog effect)
    if (fog < 1.0) {
        // Enable blending mode to allow transparent overlay
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Set fog color and transparency
        SDL_SetRenderDrawColor(renderer, COLOR_FOG.r, COLOR_FOG.g, COLOR_FOG.b, (Uint8)((1.0 - fog) * 255.0));

        // Define fog overlay rectangle
        SDL_Rect rect = { (int)x, (int)y, (int)width, (int)height };

        // Draw the fog rectangle with the calculated color and transparency
        SDL_RenderFillRect(renderer, &rect);

        // Reset blend mode to default after rendering fog
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
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
