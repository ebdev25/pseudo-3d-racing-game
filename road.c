// road.c
#include "game.h"
#include "road.h"
#include "util.h"
#include "level.h"
#include <stdlib.h>
#include <math.h>

// Define constants for Road
#define ROAD_LENGTH_NONE   0
#define ROAD_LENGTH_SHORT  25
#define ROAD_LENGTH_MEDIUM 50
#define ROAD_LENGTH_LONG   100

#define ROAD_HILL_NONE   0
#define ROAD_HILL_LOW    20
#define ROAD_HILL_MEDIUM 40
#define ROAD_HILL_HIGH   60

#define ROAD_CURVE_NONE   0
#define ROAD_CURVE_EASY   2
#define ROAD_CURVE_MEDIUM 4
#define ROAD_CURVE_HARD   6

// Returns the Y-coordinate of the last segment in the road.
// This function is useful for determining the starting elevation
// of new segments added to the road, ensuring continuity and
// smooth transitions between segments in the game's 3D space.
double lastY(Game* game) {
    if (game->road.segmentCount == 0) {
        return 0;
    } else {
        return game->road.segments[game->road.segmentCount - 1].p2.world.y;
    }
}

// Adds a new segment to the road with specified curve and end elevation (y).
// This function allocates memory for the new segment, sets up its 3D position
// and properties, and updates the road segment count.

void addSegment(Game* game, double curve, double y) {
    int n = game->road.segmentCount;

    // Allocate memory for the new segment in the segments array.
    game->road.segments = realloc(game->road.segments, sizeof(Segment) * (n + 1));
    Segment* segment = &game->road.segments[n];
    segment->index = n;

    // Set the world coordinates for the starting point (p1) of the new segment.
    segment->p1.world.x = 0;                    // X-position is constant
    segment->p1.world.y = lastY(game);          // Start Y-position continues from the last segment's end
    segment->p1.world.z = n * SEGMENT_LENGTH;   // Z-position based on segment index

    // Set the world coordinates for the ending point (p2) of the new segment.
    segment->p2.world.x = 0;
    segment->p2.world.y = y;                    // End Y-position as specified by the function argument
    segment->p2.world.z = (n + 1) * SEGMENT_LENGTH;

    // Set the curvature for the segment to control road bending.
    segment->curve = curve;

    // Assign alternating colors based on rumble length to create visual lane markers.
    segment->color = ((n / game->rumbleLength) % 2 == 0) ? COLOR_LIGHT : COLOR_DARK;

    // Initialize sprites and cars arrays for this segment.
    segment->sprites = NULL;
    segment->spriteCount = 0;
    segment->cars = NULL;
    segment->carCount = 0;

    // Increment the total count of road segments.
    game->road.segmentCount++;
}

// Adds a sprite to a specific segment of the road at a specified offset.
// This function ensures that the segment index is valid, then allocates memory
// for the new sprite in the segment and initializes its properties.

void addSprite(Game* game, int segmentIndex, const Sprite* sprite, double offset) {
    // Check if the provided segment index is within valid bounds.
    if (segmentIndex < 0 || segmentIndex >= game->road.segmentCount)
        return; // Exit if the segment index is invalid.

    // Retrieve the segment at the specified index.
    Segment* segment = &game->road.segments[segmentIndex];

    // Allocate memory for a new sprite in the segment's sprites array.
    segment->sprites = realloc(segment->sprites, sizeof(SpriteInstance) * (segment->spriteCount + 1));

    // Initialize the new sprite instance with the provided sprite and offset.
    SpriteInstance* spriteInstance = &segment->sprites[segment->spriteCount];
    spriteInstance->source = sprite;  // Assign the sprite's texture or image
    spriteInstance->offset = offset;  // Set the sprite's horizontal offset on the road

    // Increment the sprite count for the segment.
    segment->spriteCount++;
}

// Adds a series of segments to create a road section with specified entry, hold, and exit phases.
// Each phase has customizable curvature and elevation, allowing smooth transitions for curves and hills.

void addRoad(Game* game, int enter, int hold, int leave, double curve, double y) {
    // Calculate the starting and ending heights of this road section.
    double startY = lastY(game);
    double endY = startY + (y * SEGMENT_LENGTH);

    // Total number of segments in this road section.
    int total = enter + hold + leave;
    int n;

    // Entry phase: gradually increase the curve and elevation to target values.
    for (n = 0; n < enter; n++) {
        double percent = (double)n / enter;                // Progress of entry phase as a percentage
        double c = easeIn(0, curve, percent);              // Gradually increase curvature using easing
        double yPos = easeInOut(startY, endY, (double)n / total); // Smooth transition for height
        addSegment(game, c, yPos);                         // Add the segment with calculated curve and elevation
    }

    // Hold phase: maintain the specified curve and elevation across segments.
    for (n = 0; n < hold; n++) {
        double yPos = easeInOut(startY, endY, (double)(enter + n) / total); // Smoothly continue elevation
        addSegment(game, curve, yPos);                    // Add segment with consistent curve and elevation
    }

    // Exit phase: gradually decrease the curve and elevation back to zero.
    for (n = 0; n < leave; n++) {
        double percent = (double)n / leave;               // Progress of exit phase as a percentage
        double c = easeInOut(curve, 0, percent);          // Gradually reduce curvature to zero using easing
        double yPos = easeInOut(startY, endY, (double)(enter + hold + n) / total); // Smooth transition for height
        addSegment(game, c, yPos);                        // Add the segment with decreasing curve and elevation
    }
}

// Adds a straight road section with a specified length.
void addStraight(Game* game, int num) {
    num = (num == 0) ? ROAD_LENGTH_MEDIUM : num;
    addRoad(game, num, num, num, 0, 0);
}

// Adds a hill with specified length and height.
void addHill(Game* game, int num, double height) {
    num = (num == 0) ? ROAD_LENGTH_MEDIUM : num;
    height = (height == 0) ? ROAD_HILL_MEDIUM : height;
    addRoad(game, num, num, num, 0, height);
}

// Adds a curved road section with specified curvature and height.
void addCurve(Game* game, int num, double curve, double height) {
    num = (num == 0) ? ROAD_LENGTH_MEDIUM : num;
    curve = (curve == 0) ? ROAD_CURVE_MEDIUM : curve;
    height = (height == 0) ? ROAD_HILL_NONE : height;
    addRoad(game, num, num, num, curve, height);
}

// Adds a series of gentle hills with optional curvature for rolling terrain.
void addLowRollingHills(Game* game, int num, double height) {
    num = (num == 0) ? ROAD_LENGTH_SHORT : num;
    height = (height == 0) ? ROAD_HILL_LOW : height;

    addRoad(game, num, num, num, 0, height / 2);
    addRoad(game, num, num, num, 0, -height);
    addRoad(game, num, num, num, ROAD_CURVE_EASY, height);
    addRoad(game, num, num, num, 0, 0);
    addRoad(game, num, num, num, -ROAD_CURVE_EASY, height / 2);
    addRoad(game, num, num, num, 0, 0);
}

// Adds a series of S-curves with varying curvature and elevation.
void addSCurves(Game* game) {
    int num = ROAD_LENGTH_MEDIUM;

    addRoad(game, num, num, num, -ROAD_CURVE_EASY, ROAD_HILL_NONE);
    addRoad(game, num, num, num, ROAD_CURVE_MEDIUM, ROAD_HILL_MEDIUM);
    addRoad(game, num, num, num, ROAD_CURVE_EASY, -ROAD_HILL_LOW);
    addRoad(game, num, num, num, -ROAD_CURVE_EASY, ROAD_HILL_MEDIUM);
    addRoad(game, num, num, num, -ROAD_CURVE_MEDIUM, -ROAD_HILL_MEDIUM);
}

// Adds a bumpy road section with alternating elevation changes.
void addBumps(Game* game) {
    addRoad(game, 10, 10, 10, 0, 5);
    addRoad(game, 10, 10, 10, 0, -2);
    addRoad(game, 10, 10, 10, 0, -5);
    addRoad(game, 10, 10, 10, 0, 8);
    addRoad(game, 10, 10, 10, 0, 5);
    addRoad(game, 10, 10, 10, 0, -7);
    addRoad(game, 10, 10, 10, 0, 5);
    addRoad(game, 10, 10, 10, 0, -2);
}

// Adds a downhill road section that gradually descends to the end of the track.
void addDownhillToEnd(Game* game, int num) {
    num = (num == 0) ? 200 : num;
    double height = -lastY(game) / SEGMENT_LENGTH;
    addRoad(game, num, num, num, -ROAD_CURVE_EASY, height);
}

void resetRoad(Game* game) {
    // Free existing segments to reset the road layout
    for (int i = 0; i < game->road.segmentCount; i++) {
        free(game->road.segments[i].sprites);  // Free any sprites in each segment
        free(game->road.segments[i].cars);     // Free any cars in each segment
    }
    free(game->road.segments);                // Free the segment array
    game->road.segments = NULL;               // Reset segment pointer to NULL
    game->road.segmentCount = 0;              // Reset the segment count
    // 2) store the loaded commands in game->loadedRoadData
    LevelRoadData* rd = &game->loadedRoadData;
    // If no commands, fallback to default:
    if (rd->commandCount <= 0) {
        SDL_Log("resetRoad: No road commands found; building default track.\n");
        // add func call here for building default road
    } else {
        SDL_Log("resetRoad: Using loaded road commands.\n");
        for (int i = 0; i < rd->commandCount; i++) {
            RoadCommand* cmd = &rd->commands[i];
            // interpret the commands
            if (strcmp(cmd->type, "straight") == 0) {
                // param[0] is the "num"
                int num = (cmd->paramCount > 0) ? (int)cmd->params[0] : 0;
                addStraight(game, num);
            }
            else if (strcmp(cmd->type, "hill") == 0) {
                // param[0] => num, param[1] => height
                int num = (cmd->paramCount > 0) ? (int)cmd->params[0] : 0;
                double height = (cmd->paramCount > 1) ? cmd->params[1] : 0.0;
                addHill(game, num, height);
            }
            else if (strcmp(cmd->type, "curve") == 0) {
                // param[0] => num, param[1] => curve, param[2] => height
                int num = (cmd->paramCount > 0) ? (int)cmd->params[0] : 0;
                double curveVal = (cmd->paramCount > 1) ? cmd->params[1] : 0.0;
                double height = (cmd->paramCount > 2) ? cmd->params[2] : 0.0;
                addCurve(game, num, curveVal, height);
            }
            else if (strcmp(cmd->type, "lowRollingHills") == 0) {
                // param[0] => num, param[1] => height
                int num = (cmd->paramCount > 0) ? (int)cmd->params[0] : 0;
                double height = (cmd->paramCount > 1) ? cmd->params[1] : 0.0;
                addLowRollingHills(game, num, height);
            }
            else if (strcmp(cmd->type, "sCurves") == 0) {
                // no params needed
                addSCurves(game);
            }
            else if (strcmp(cmd->type, "bumps") == 0) {
                addBumps(game);
            }
            else if (strcmp(cmd->type, "downhillToEnd") == 0) {
                // param[0] => num
                int num = (cmd->paramCount > 0) ? (int)cmd->params[0] : 0;
                addDownhillToEnd(game, num);
            }
            else {
                SDL_Log("resetRoad: Unknown command '%s'. Skipping.\n", cmd->type);
            }
        }
    }

    //addDownhillToEnd(game, 0);

    // Reset road sprites and cars to their default positions
    resetSprites(game);                             // Place scenery objects along the track
    resetCars(game);                                // Populate the track with cars

    // Define the start and finish line colors for track segments
    Segment* segment = findSegment(game, game->playerZ);
    if (segment) {
        int idx = segment->index;
        // Set two segments after the player position as the start line
        if (idx + 2 < game->road.segmentCount)
            game->road.segments[idx + 2].color = COLOR_START;
        if (idx + 3 < game->road.segmentCount)
            game->road.segments[idx + 3].color = COLOR_START;
    }
    
    // Set the last few segments as the finish line
    for (int i = 0; i < game->rumbleLength; i++) {
        if (game->road.segmentCount - 1 - i >= 0)
            game->road.segments[game->road.segmentCount - 1 - i].color = COLOR_FINISH;
    }

    // Calculate the total track length based on segment count
    game->road.trackLength = game->road.segmentCount * SEGMENT_LENGTH;

    // Set lodCycleThreshold based on track length and LOD phases
    game->lodCycleThreshold = game->road.trackLength / 3;
}

void resetSprites(Game* game) {
    int n, i;
    int totalSegments = game->road.segmentCount;
    //addSprite(game, 16, &SPRITE_LIGHT_FRAME1, -1.2);

    // Add billboards to specific points on either side of the track
    addSprite(game, 240, &SPRITE_BILLBOARD07, -1.2);
    addSprite(game, 240, &SPRITE_BILLBOARD06, 1.2);
    addSprite(game, totalSegments - 25, &SPRITE_BILLBOARD07, -1.2);
    addSprite(game, totalSegments - 25, &SPRITE_BILLBOARD06, 1.2);
    /*
    // Add palm trees at intervals along the track, with random offsets for variation
    for (n = 10; n < 200; n += 4 + (int)(n / 100)) {
        addSprite(game, n, &SPRITE_PALM_TREE, 0.5 + ((double)rand() / RAND_MAX) * 0.5);   // Random offset near the road
        addSprite(game, n, &SPRITE_PALM_TREE, 1.0 + ((double)rand() / RAND_MAX) * 2.0);   // Offset further away
    }

    // Add columns and trees at intervals between segments 250 and 1000
    for (n = 250; n < 1000; n += 5) {
        addSprite(game, n, &SPRITE_COLUMN, 1.1);    // Place columns on one side
        addSprite(game, n + rand() % 6, &SPRITE_TREE1, -1 - ((double)rand() / RAND_MAX) * 2.0); // Randomized position for TREE1
        addSprite(game, n + rand() % 6, &SPRITE_TREE2, -1 - ((double)rand() / RAND_MAX) * 2.0); // Randomized position for TREE2
    }

    // Add random plants throughout the rest of the segments with variable offsets
    for (n = 200; n < totalSegments; n += 3) {
        const Sprite* plantSprite = SPRITES_PLANTS[rand() % PLANT_COUNT]; // Random plant sprite from the list
        double offset = (rand() % 2 ? 1 : -1) * (2 + ((double)rand() / RAND_MAX) * 5); // Random offset, either left or right
        addSprite(game, n, plantSprite, offset);
    }
    */
    // Randomly add billboards and clusters of plants in higher segments for added density
    int side;
    const Sprite* sprite;
    double offset;
    double const BILLBOARD_OFFSET = 1.2;

    for (n = 1000; n < (totalSegments - 50); n += 100) {
        side = rand() % 2 ? 1 : -1; // Randomly choose left or right side of the road
        int randOffset = rand() % 51; // Random offset for billboard placement within segment range
        sprite = SPRITES_BILLBOARDS[rand() % BILLBOARD_COUNT]; // Random billboard sprite
        addSprite(game, n + randOffset, sprite, -side * BILLBOARD_OFFSET); // Add billboard with chosen side and offset
        /*
        // Add a cluster of plants around each billboard for added scenery
        for (i = 0; i < 20; i++) {
            sprite = SPRITES_PLANTS[rand() % PLANT_COUNT]; // Random plant sprite from list
            offset = side * (1.5 + ((double)rand() / RAND_MAX)); // Offset plants slightly from the road
            randOffset = rand() % 51; // Varying position within the cluster
            addSprite(game, n + randOffset, sprite, offset);
        }
        */
    }
}

void resetCars(Game* game) {
    int n;
    // Free any existing car data in the road struct
    free(game->road.cars);
    game->road.cars = NULL;
    game->road.carCount = 0;
    /*
    // Add a set number of cars to the track, defined by game->totalCars
    for (n = 0; n < game->totalCars; n++) {
        // Allocate memory for a new car
        Car* car = malloc(sizeof(Car));
        
        // Set the car's lateral offset position. 
        // Randomly places the car on the left or right lane within a set range.
        car->offset = ((double)rand() / RAND_MAX) * (rand() % 2 ? -0.8 : 0.8);
        
        // Position the car along the track by randomly assigning it to a segment
        car->z = (rand() % game->road.segmentCount) * SEGMENT_LENGTH;
        
        // Randomly select a car sprite from the list of available car sprites
        car->sprite = SPRITES_CARS[rand() % CAR_COUNT];
        
        // Assign a speed to the car, with slower speeds for larger vehicles (like semis)
        car->speed = game->maxSpeed / 4 + ((double)rand() / RAND_MAX) * game->maxSpeed / ((car->sprite == &SPRITE_SEMI) ? 4 : 2);
        
        // Initial percent on the segment (for position interpolation)
        car->percent = 0;

        // Locate the segment the car is on and add the car to this segment's car list
        Segment* segment = findSegment(game, car->z);
        segment->cars = realloc(segment->cars, sizeof(Car*) * (segment->carCount + 1));
        segment->cars[segment->carCount] = car;
        segment->carCount++;

        // Add the car to the road's main car array for overall tracking
        game->road.cars = realloc(game->road.cars, sizeof(Car) * (game->road.carCount + 1));
        game->road.cars[game->road.carCount] = *car;
        game->road.carCount++;
    }
    */
}


 // Finds and returns the segment of the road that corresponds to a given position.
 // This function calculates the segment index based on the position and the
 // fixed length of each segment.
 
 // game Pointer to the Game structure containing road segments.
 // position The position on the road for which the segment is needed.
 // returns Pointer to the segment corresponding to the specified position.

Segment* findSegment(Game* game, double position) {
    int index = (int)(position / SEGMENT_LENGTH) % game->road.segmentCount;
    return &game->road.segments[index];
}

double calculateElevation(Segment* segment, double position) {
    double segmentPosition = fmod(position, SEGMENT_LENGTH) / SEGMENT_LENGTH;
    return segment->p1.world.y + (segment->p2.world.y - segment->p1.world.y) * segmentPosition;
}