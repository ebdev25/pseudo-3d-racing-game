// render.c
#include "render.h"
#include "game.h"
#include "util.h"
#include "constants.h"

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

    // Retrieve current background cycle state
    int phase = game->backgroundPhase;
    const BackgroundCycleState* currentState = &BACKGROUND_CYCLE_STATES[phase];

    // Render the sky layer (static background, no parallax or transitions)
    renderBackground(renderer, background, width, height, &BACKGROUND_SKY.highLOD, game->skyOffset, &SKY_OFFSET, 0, false);

    if (game->slidingWindow->transitioning) {
        // Get current and target background cycle states
        int currentPhase = game->slidingWindow->currentStateIndex;
        const BackgroundCycleState* currentState = &BACKGROUND_CYCLE_STATES[currentPhase];
        int targetPhase = game->slidingWindow->targetStateIndex;
        const BackgroundCycleState* targetState = &BACKGROUND_CYCLE_STATES[targetPhase];

        // Calculate eased progress for smooth transitions
        double easedProgress = easeInOut(0.0, 1.0, (double)game->slidingWindow->progress);

        // Calculate dynamic offsets
        // Outgoing background scrolls left from 0 to -cycleStateWidth
        double outgoingOffsetX = -game->cycleStateWidth * easedProgress;

        // Incoming background scrolls in from right (cycleStateWidth) to 0
        double incomingOffsetX = game->cycleStateWidth * (1.0 - easedProgress);

        // Log the offsets for debugging
        SDL_Log("Render - Transition Progress: %.2f, Eased Progress: %.2f, Incoming Offset: %.2f, Outgoing Offset: %.2f",
                game->slidingWindow->progress, easedProgress, incomingOffsetX, outgoingOffsetX);

        // Render incoming background layers first (behind outgoing background)
        // Ensure wrapEnabled is false to prevent tiling during transition
        renderBackground(renderer, background, width, height, targetState->L2, 0.0, targetState->L2Offset, (int)incomingOffsetX, false);
        renderBackground(renderer, background, width, height, targetState->L3, 0.0, targetState->L3Offset, (int)incomingOffsetX, false);
        renderBackground(renderer, background, width, height, targetState->L4, 0.0, targetState->L4Offset, (int)incomingOffsetX, false);

        // Render outgoing background layers on top
        // Ensure wrapEnabled is false to prevent tiling during transition
        renderBackground(renderer, background, width, height, currentState->L2, 0.0, currentState->L2Offset, (int)outgoingOffsetX, false);
        renderBackground(renderer, background, width, height, currentState->L3, 0.0, currentState->L3Offset, (int)outgoingOffsetX, false);
        renderBackground(renderer, background, width, height, currentState->L4, 0.0, currentState->L4Offset, (int)outgoingOffsetX, false);
    }
    else {
        // No transition: Render current background layers normally with tiling enabled
        int currentPhase = game->slidingWindow->currentStateIndex;
        const BackgroundCycleState* currentState = &BACKGROUND_CYCLE_STATES[currentPhase];

        // Render each background layer with tiling enabled
        renderBackground(renderer, background, width, height, currentState->L2, game->hillOffset, currentState->L2Offset, 0, true);
        renderBackground(renderer, background, width, height, currentState->L3, game->treeOffset, currentState->L3Offset, 0, true);
        renderBackground(renderer, background, width, height, currentState->L4, game->mountainOffset, currentState->L4Offset, 0, true);
    }

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

// Render a background layer with optional parallax lock and cycle state constraints
void renderBackground(SDL_Renderer* renderer, SDL_Texture* background, int screenWidth, int screenHeight, 
                     const SDL_Rect* layerRect, double layerOffset, const Offset* layerPositionOffset, 
                     int initialOffsetX, bool wrapEnabled) {
    // Calculate the horizontal offset in pixels
    double totalOffsetPixels = fmod(layerOffset, (double)layerRect->w);

    // Adjust for negative offsets
    if (totalOffsetPixels < 0.0) {
        totalOffsetPixels += (double)layerRect->w;
    }

    // Calculate the starting X position for rendering
    int startX = initialOffsetX - (int)totalOffsetPixels;

    if (wrapEnabled) {
        // Number of tiles needed to cover the screen width
        int tiles = (int)ceil((double)screenWidth / (double)layerRect->w) + 1;

        for (int i = 0; i < tiles; i++) {
            // Define the source rectangle (entire layer)
            SDL_Rect srcRect = *layerRect;

            // Define the destination rectangle
            SDL_Rect destRect;
            destRect.x = startX + i * layerRect->w;
            destRect.y = layerPositionOffset ? layerPositionOffset->y : 0;
            destRect.w = layerRect->w;
            destRect.h = layerRect->h;

            // Render the texture
            SDL_RenderCopy(renderer, background, &srcRect, &destRect);
        }
    }
    else {
        // Only render one tile to prevent wrap-around
        SDL_Rect srcRect = *layerRect;

        SDL_Rect destRect;
        destRect.x = startX;
        destRect.y = layerPositionOffset ? layerPositionOffset->y : 0;
        destRect.w = layerRect->w;
        destRect.h = layerRect->h;

        // Render the texture
        SDL_RenderCopy(renderer, background, &srcRect, &destRect);
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

    // Apply fog effect from the top to the bottom of the segment
    // renderFog(renderer, 0, y1, width, y2 - y1, fog);
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
