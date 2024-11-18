// render.c
#include "render.h"
#include "game.h"
#include "util.h"
#include <math.h>
#include <stdlib.h>

/*
 * Rendering System Overview - render.c
 *
 * This file handles the rendering of the game scene, breaking down each stage into specific functions to manage layers, road segments, sprites, and special effects like fog.
 * The rendering system in `render.c` follows these primary steps to create the visual scene:
 *
 * 1. **Background Layers**:
 *    - Renders static and scrolling background layers (sky, hills, trees) to create depth and atmosphere.
 *    - `renderBackground()` handles each layer by calculating horizontal offsets based on camera position, producing the effect of movement.
 *    - The `rotation` parameter controls the scrolling offset, simulating lateral movement.
 *
 * 2. **Road Segments**:
 *    - The road is rendered using segments to create a continuous path with dynamic curves, hills, and lane markers.
 *    - `renderSegment()` draws each road segment from back to front, layering polygons (grass, road, rumble strips, lane markers) based on depth and curve data.
 *    - A depth check avoids drawing segments that fall outside the camera’s view.
 *
 * 3. **Projection**:
 *    - Each road segment point is projected from 3D world space into 2D screen space using the `project()` function.
 *    - The position, curve, and player movement affect each point’s coordinates, adjusting their size and position to create perspective.
 *
 * 4. **Rendering Sprites**:
 *    - Sprites such as roadside objects (trees, billboards) and cars are rendered based on their position on the track.
 *    - `renderSprite()` calculates each sprite’s screen coordinates and scales them based on perspective, ensuring they appear larger up close and smaller in the distance.
 *    - Sprites are ordered back-to-front to prevent rendering overlap issues.
 *
 * 5. **Player Car**:
 *    - The player's car sprite is rendered at a fixed position on the screen, simulating movement by adjusting its bounce and orientation (left, right, uphill).
 *    - `renderPlayer()` chooses the appropriate sprite based on steering input and slope data.
 *
 * 6. **Fog Effect**:
 *    - A fog effect is applied to distant road segments to enhance depth and visual focus on nearby elements.
 *    - `renderFog()` draws a semi-transparent overlay that intensifies with distance, giving the road a faded effect in the far distance.
 *
 * 7. **Polygon Drawing Utility**:
 *    - `renderPolygon()` provides a low-level function for drawing quads with color blending, used by road and lane markers.
 *
 * The rendering functions rely on SDL for setting draw colors, blending, and copying textures to the renderer. The combination of these steps results in a dynamic, pseudo-3D road scene that responds to player input and game events.
 */

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

    // Retrieve background layer offsets and speeds for parallax effect
    double skyOffset = game->skyOffset;
    double hillOffset = game->hillOffset;
    double treeOffset = game->treeOffset;
    double housesOffset = game->housesOffset;

    // Initialize player Y-coordinate to 0; will be calculated based on segment position
    double playerY = 0;

    // Find the base segment (where the player currently is) and the interpolation value within the segment
    Segment* baseSegment = findSegment(game, position);
    double basePercent = percentRemaining(position, SEGMENT_LENGTH);

    // Locate the segment where the player will be positioned (slightly ahead) and the interpolation within it
    Segment* playerSegment = findSegment(game, position + playerZ);
    double playerPercent = percentRemaining(position + playerZ, SEGMENT_LENGTH);

    // Interpolate the player's Y-position based on the segment data
    playerY = interpolate(playerSegment->p1.world.y, playerSegment->p2.world.y, playerPercent);

    // Set the maximum Y value to the screen height for clipping purposes
    double maxY = height;

    // Initialize X position and delta X for curve effect in the road
    double x = 0;
    double dx = - (baseSegment->curve * basePercent);

    // Render the sky layer (no cycling)
    renderBackground(renderer, game->background, width, height, &BACKGROUND_SKY.highLOD, game->skyOffset, NULL, 0);

    // Get the current and next cycle states
    int phase = game->backgroundPhase;
    const BackgroundCycleState* currentState = &BACKGROUND_CYCLE_STATES[phase];

    if (game->isTransitioning) {
        int nextPhase = game->nextBackgroundPhase;
        const BackgroundCycleState* nextState = &BACKGROUND_CYCLE_STATES[nextPhase];

        double t = game->transitionProgress;  // Transition progress (0.0 to 1.0)
        int offsetXCurrent = (int)(-t * width);          // Current state moves left
        int offsetXNext = (int)((1.0 - t) * width);      // Next state moves in from right

        // DISREGARD: Zero parallax offset during transitions
        double parallaxOffset = 0.0;

        // Use fixed parallex offsets during transitions
        double hillOffset = game->fixedHillOffset;
        double treeOffset = game->fixedHousesOffset;
        double housesOffset = game->fixedHousesOffset;

        // Render current state's layers w/out individual parallax
        renderBackground(renderer, game->background, width, height, currentState->L2, hillOffset, currentState->L2Offset, offsetXCurrent);
        renderBackground(renderer, game->background, width, height, currentState->L3, treeOffset, currentState->L3Offset, offsetXCurrent);
        renderBackground(renderer, game->background, width, height, currentState->L4, housesOffset, currentState->L4Offset, offsetXCurrent);

        // Render next state's layers w/out individual parallax
        renderBackground(renderer, game->background, width, height, nextState->L2, hillOffset, nextState->L2Offset, offsetXNext);
        renderBackground(renderer, game->background, width, height, nextState->L3, treeOffset, nextState->L3Offset, offsetXNext);
        renderBackground(renderer, game->background, width, height, nextState->L4, housesOffset, nextState->L4Offset, offsetXNext);
    } else {
        // Render current state layers normally
        renderBackground(renderer, game->background, width, height, currentState->L2, game->hillOffset, currentState->L2Offset, 0);
        renderBackground(renderer, game->background, width, height, currentState->L3, game->treeOffset, currentState->L3Offset, 0);
        renderBackground(renderer, game->background, width, height, currentState->L4, game->housesOffset, currentState->L4Offset, 0);
    }

    // Background specific fog effect
    if (game->fogEnabled && game->fogIntensity > 0.0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, game->fogColor.r, game->fogColor.g, game->fogColor.b, (Uint8)(game->fogIntensity * 255));
        SDL_Rect fogRect = {0,0, width, height};
        SDL_RenderFillRect(renderer, &fogRect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    
    // Loop through each segment in the visible range
    for (int n = 0; n < drawDistance; n++) {
        // Calculate the current segment index, accounting for looping if beyond track end
        Segment* segment = &game->road.segments[(baseSegment->index + n) % game->road.segmentCount];
        int looped = segment->index < baseSegment->index ? 1 : 0;

        // Apply exponential fog to create a depth effect as distance increases
        double fog = exponentialFog((double)n / drawDistance, fogDensity);
        
        // Set clipping limit to the current max Y
        segment->clip = maxY;

        // Calculate camera-relative coordinates for projecting segment points
        double cameraX = playerX * roadWidth - x;
        double cameraY = playerY + cameraHeight;
        double cameraZ = position - (looped ? game->road.trackLength : 0);

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
        Segment* segment = &game->road.segments[(baseSegment->index + n) % game->road.segmentCount];

        // Render each car in the segment
        for (int i = 0; i < segment->carCount; i++) {
            Car* car = segment->cars[i];
            const Sprite* sprite = car->sprite;

            // Interpolate scale and screen position based on segment's points
            double spriteScale = interpolate(segment->p1.scale, segment->p2.scale, car->percent);
            double spriteX = interpolate(segment->p1.screen.x, segment->p2.screen.x, car->percent) +
                             (spriteScale * car->offset * roadWidth * width / 2);
            double spriteY = interpolate(segment->p1.screen.y, segment->p2.screen.y, car->percent);

            // Render the car sprite with calculated properties
            renderSprite(game, spritesheet, sprite, spriteScale, spriteX, spriteY, -0.5, -1, segment->clip);
        }

        // Render each roadside sprite (e.g., trees, billboards) in the segment
        for (int i = 0; i < segment->spriteCount; i++) {
            SpriteInstance* spriteInstance = &segment->sprites[i];
            const Sprite* sprite = spriteInstance->source;
            double spriteScale = segment->p1.scale;
            double spriteX = segment->p1.screen.x +
                             (spriteScale * spriteInstance->offset * roadWidth * width / 2);
            double spriteY = segment->p1.screen.y;

            // Render the roadside sprite with calculated properties
            renderSprite(game, spritesheet, sprite, spriteScale, spriteX, spriteY,
                         (spriteInstance->offset < 0 ? -1 : 0), -1, segment->clip);
        }

        // Render the player sprite in the current segment if it's the player segment
        if (segment == playerSegment) {
            double speedPercent = speed / maxSpeed;
            double playerScale = cameraDepth / playerZ;
            double playerDestX = width / 2;
            
            // Calculate the vertical position for player sprite with slight offset
            double playerDestY = (height / 2) - 
                                 (playerScale * interpolate(playerSegment->p1.camera.y, 
                                                            playerSegment->p2.camera.y, 
                                                            playerPercent) * height / 2) - 150;

            // Determine steering and elevation change for rendering player
            double steer = (game->keyLeft ? -1 : game->keyRight ? 1 : 0);
            double updown = playerSegment->p2.world.y - playerSegment->p1.world.y;

            // Render the player car
            renderPlayer(game, speedPercent, playerScale, playerDestX, playerDestY, steer, updown);
        }
    }
}
/*
void renderBackground(SDL_Renderer* renderer, SDL_Texture* background, int width, int height, const BackgroundLayer* layer, double rotation, double offset) {
    // Set default rotation to 0 if not provided
    rotation = rotation ? rotation : 0;

    // `rotation` determines the horizontal offset in the background layer to create a scrolling effect.
    // It is used here to simulate movement in the background, creating a parallax effect as the
    // player's perspective changes. Each background layer has its own scroll speed and this rotation
    // controls how much each layer "scrolls."

    offset = offset ? offset : 0; // Set default offset to 0 if not provided

    // Get background layer image dimensions (half of full layer width to fit the screen)
    int imageW = layer->w / 2;
    int imageH = layer->h;

    // Calculate starting x position in the source texture, shifted by `rotation` for scrolling
    int sourceX = layer->x + (int)(layer->w * rotation) % layer->w;
    int sourceY = layer->y;

    // Calculate the width of the source rectangle, ensuring it wraps if it exceeds the layer width
    int sourceW = (imageW < layer->x + layer->w - sourceX) ? imageW : layer->x + layer->w - sourceX;
    int sourceH = imageH;

    // Define the source rectangle for texture extraction based on `rotation` and layer dimensions
    SDL_Rect srcRect = {sourceX, sourceY, sourceW, sourceH};

    // Define destination rectangle, stretching width proportionally and adjusting height
    SDL_Rect destRect = {0, (int)offset, (int)(width * ((double)sourceW / imageW)), height};

    // Render the main portion of the background layer
    SDL_RenderCopy(renderer, background, &srcRect, &destRect);

    // If the texture doesn't fully cover the screen, render the remaining portion by wrapping around
    if (sourceW < imageW) {
        srcRect.x = layer->x;            // Start from the beginning of the layer
        srcRect.w = imageW - sourceW;     // Set width for remaining part of the background
        destRect.x = destRect.w - 1;      // Position the remaining part at the edge of the last render
        destRect.w = width - destRect.w;  // Set width to fill the rest of the screen

        SDL_RenderCopy(renderer, background, &srcRect, &destRect);  // Render the wrapped portion
    }
}
*/

void renderBackground(SDL_Renderer* renderer, SDL_Texture* background, int screenWidth, int screenHeight, 
                      const SDL_Rect* layerRect, double layerOffset, const Offset* layerPositionOffset, int offsetX) {
    // layerRect: Rectangle defining the portion of the texture to render
    // layerOffset: Parallax offset for the layer
    // layerPositionOffset: Vertical offset for the layer
    // offsetX: Horizontal offset for transition effects

    // Calculate total horizontal offset, including parallax and transition
    int layerWidth = layerRect->w;
    double totalOffsetPixels = fmod(layerOffset * layerWidth, layerWidth);

    // Adjust for wrapping
    if (totalOffsetPixels < 0) {
        totalOffsetPixels += layerWidth;
    }

    // Compute source rectangle
    SDL_Rect srcRect;
    srcRect.y = layerRect->y;
    srcRect.h = layerRect->h;
    srcRect.x = (int)(layerRect->x + totalOffsetPixels) % layerWidth;
    srcRect.w = (int)fmin(layerWidth - srcRect.x, screenWidth);

    // Compute destination rectangle
    SDL_Rect destRect;
    destRect.x = offsetX;
    destRect.y = layerPositionOffset ? layerPositionOffset->y : 0;
    destRect.w = srcRect.w;
    destRect.h = screenHeight;

    // Render the first part
    SDL_RenderCopy(renderer, background, &srcRect, &destRect);

    // Handle wrapping if necessary
    if (srcRect.w < screenWidth) {
        // Second part wraps around to the beginning
        srcRect.x = layerRect->x;
        srcRect.w = layerWidth - srcRect.w;

        destRect.x += destRect.w;
        destRect.w = srcRect.w;

        SDL_RenderCopy(renderer, background, &srcRect, &destRect);
    }
}

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
        lanew1 = w1 * 2 / lanes;  // Width of each lane at the top of the segment
        lanew2 = w2 * 2 / lanes;  // Width of each lane at the bottom of the segment
        lanex1 = x1 - w1 + lanew1; // Starting x position for lane marker
        lanex2 = x2 - w2 + lanew2;

        // Draw each lane marker as a narrow polygon strip
        for (int lane = 1; lane < lanes; lane++, lanex1 += lanew1, lanex2 += lanew2) {
            renderPolygon(renderer,
                          lanex1 - l1 / 2, y1,
                          lanex1 + l1 / 2, y1,
                          lanex2 + l2 / 2, y2,
                          lanex2 - l2 / 2, y2,
                          &color->lane);
        }
    }

    // Apply fog effect from the top to the bottom of the segment
    //renderFog(renderer, 0, y1, width, y2 - y1, fog);
}

void renderPolygon(SDL_Renderer* renderer, double x1, double y1, double x2, double y2,
                   double x3, double y3, double x4, double y4, const Color* color) {
    // Define an array of 4 vertices representing the corners of the polygon
    SDL_Vertex verts[4] = {
        // Vertex 1 at (x1, y1) with the specified color
        { { (float)x1, (float)y1 }, { color->r, color->g, color->b, color->a}, { 0, 0 } },
        
        // Vertex 2 at (x2, y2) with the specified color
        { { (float)x2, (float)y2 }, { color->r, color->g, color->b, color->a}, { 0, 0 } },
        
        // Vertex 3 at (x3, y3) with the specified color
        { { (float)x3, (float)y3 }, { color->r, color->g, color->b, color->a}, { 0, 0 } },
        
        // Vertex 4 at (x4, y4) with the specified color
        { { (float)x4, (float)y4 }, { color->r, color->g, color->b, color->a}, { 0, 0 } }
    };

    // Define the order of indices to draw two triangles for a quad
    int indices[6] = { 0, 1, 2, 2, 3, 0 };
    
    // Render the polygon as a quad using SDL_RenderGeometry
    // - The `verts` array specifies the four corners of the polygon in the vertex order.
    // - `indices` specifies the drawing order: {0, 1, 2} for the first triangle
    //   and {2, 3, 0} for the second triangle, forming a complete quad.
    // - NULL is passed for the texture parameter since we’re drawing a solid color polygon.
    SDL_RenderGeometry(renderer, NULL, verts, 4, indices, 6);

    // Check if the rendering operation was successful
    // - SDL_RenderGeometry returns 0 on success and -1 on error.
    // - If an error occurs, SDL_GetError provides a descriptive message.
    if (SDL_RenderGeometry(renderer, NULL, verts, 4, indices, 6) != 0) {
        printf("SDL_RenderGeometry error: %s\n", SDL_GetError());
    }
}

void renderSprite(Game* game, SDL_Texture* spritesheet, const Sprite* sprite, double scale,
                  double destX, double destY, double offsetX, double offsetY, double clipY) {
    SDL_Renderer* renderer = game->renderer;
    int width = game->width;
    int height = game->height;

    // Dimensions of the original sprite
    int spriteWidth = sprite->w;
    int spriteHeight = sprite->h;

    // Calculate scaled width and height for the sprite based on road perspective and scale factors
    double destW = ((double)spriteWidth * scale * SPRITE_SCALE * width / 2) * (ROAD_WIDTH);
    double destH = ((double)spriteHeight * scale * SPRITE_SCALE * width / 2) * (ROAD_WIDTH);

    // Adjust the destination X and Y based on offset values
    // - `offsetX` and `offsetY` help to position the sprite relative to its base position
    // - These offsets, when non-zero, shift the sprite horizontally or vertically
    destX = destX + destW * (offsetX ? offsetX : 0);
    destY = destY + destH * (offsetY ? offsetY : 0);

    // Calculate clipping for the Y-axis to avoid rendering parts outside viewable area
    // - `clipY` is the maximum Y-coordinate where rendering is allowed
    // - Calculate `clipH` as the portion of the sprite height that would exceed `clipY`
    // - If the clipped height (`clipH`) is less than the sprite’s destination height, render it
    double clipH = clipY ? fmax(0, destY + destH - clipY) : 0;
    if (clipH < destH) {
        // Define the source rectangle on the spritesheet for the un-clipped portion
        SDL_Rect srcRect = { sprite->x, sprite->y, sprite->w, (int)(sprite->h - (sprite->h * clipH / destH)) };
        
        // Define the destination rectangle on the screen, applying scaling and clipping
        SDL_Rect destRect = { (int)destX, (int)destY, (int)destW, (int)(destH - clipH) };

        // Copy the portion of the sprite from the spritesheet to the screen
        SDL_RenderCopy(renderer, spritesheet, &srcRect, &destRect);
    }
}

void renderPlayer(Game* game, double speedPercent, double scale, double destX, double destY, double steer, double updown) {
    SDL_Texture* spritesheet = game->spritesheet;

    // Calculate a "bounce" effect based on speed and a random factor
    // - Bounce makes the player's car shift slightly up or down at different points
    // - Random factor introduces small unpredictable movements, simulating the car's vibration
    double bounce = (1.5 * ((double)rand() / RAND_MAX) * speedPercent * game->resolution) * ((rand() % 2 == 0) ? -1 : 1);

    const Sprite* sprite;

    // Determine which sprite to use based on steering and elevation change
    // - `steer < 0`: Player is steering left; check if car is going uphill or flat
    // - `steer > 0`: Player is steering right; check if car is going uphill or flat
    // - Otherwise, choose between a straight uphill or flat sprite
    if (steer < 0)
        sprite = (updown > 0) ? &SPRITE_PLAYER_UPHILL_LEFT : &SPRITE_PLAYER_LEFT;
    else if (steer > 0)
        sprite = (updown > 0) ? &SPRITE_PLAYER_UPHILL_RIGHT : &SPRITE_PLAYER_RIGHT;
    else
        sprite = (updown > 0) ? &SPRITE_PLAYER_UPHILL_STRAIGHT : &SPRITE_PLAYER_STRAIGHT;

    // Render the player sprite with position and bounce adjustments
    // - `scale` controls the size relative to distance from the camera
    // - `destX` and `destY + bounce` position the sprite horizontally and vertically
    // - Offsets of `-0.5` and `-1` center the sprite over the road and apply vertical offset
    renderSprite(game, spritesheet, sprite, scale, destX, destY + bounce, -0.5, -1, 0);
}

void renderFog(SDL_Renderer* renderer, double x, double y, double width, double height, double fog) {
    // Only render fog if the fog density is below 1 (visible fog effect)
    if (fog < 1) {
        // Enable blending mode to allow transparent overlay
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Set fog color and transparency
        // - Fog color is set with `COLOR_FOG`, usually a greyish tone
        // - `fog` determines the transparency, ranging from fully opaque (fog = 0) to fully transparent (fog = 1)
        SDL_SetRenderDrawColor(renderer, COLOR_FOG.r, COLOR_FOG.g, COLOR_FOG.b, (Uint8)((1 - fog) * 255));

        // Define fog overlay rectangle
        // - Rectangle covers the segment from `x, y` with specified width and height
        SDL_Rect rect = { (int)x, (int)y, (int)width, (int)height };

        // Draw the fog rectangle with the calculated color and transparency
        SDL_RenderFillRect(renderer, &rect);

        // Reset blend mode to default after rendering fog
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

// Calculates the width of the rumble strip based on the projected road width and lane count.
// - `projectedRoadWidth` is the apparent width of the road at a given distance, after perspective projection.
// - The width of the rumble strip decreases as the number of lanes increases, with a minimum divisor of 6 to prevent overly thin strips.
double rumbleWidth(double projectedRoadWidth, int lanes) {
    return projectedRoadWidth / fmax(6, 2 * lanes);
}

// Calculates the width of the lane markers based on the projected road width and lane count.
// - `projectedRoadWidth` is used to determine the lane marker width in proportion to the visible road width.
// - The lane marker width reduces as lane count increases, with a minimum divisor of 32 to ensure visibility at higher lane counts.
double laneMarkerWidth(double projectedRoadWidth, int lanes) {
    return projectedRoadWidth / fmax(32, 8 * lanes);
}
