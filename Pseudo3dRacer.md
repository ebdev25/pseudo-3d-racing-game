# Pseudo-3D Racing Game

## Overview

The Pseudo-3D Racing Game is a retro-inspired racing game built with SDL2. It uses parallax scrolling and layered background rendering to create an immersive experience, complete with dynamic transitions between background textures. The game incorporates a sliding window mechanism for seamless background transitions and includes advanced parallax effects for a realistic 3D feel.

----------

## Features

-   **Dynamic Background Rendering:** Smooth transitions between biomes with multiple levels of detail (LOD).
    
-   **Parallax Scrolling:** Separate layers for hills, trees, mountains, and sky, with independent scrolling speeds for depth simulation.
    
-   **Fully Configurable:** Adjustable gameplay parameters for speed, acceleration, braking, and more.
----------
# **Technical Documentation**
Technical documentation overview of the files that make up the Pseudo-3d Racing Game project.
##
`constants.h` and `constants.c`
## **Overview**

The `constants.h` and `constants.c` files define constants, structures, and utility functions used across the pseudo-3D racing game. These files provide essential components such as:

1.  **Struct Definitions**: Structures for points, colors, sprites, background layers, and transitions.
2.  **Constants**: Game-wide constants such as resolution, physics properties, and rendering parameters.
3.  **Global Variables**: Predefined values for colors, sprites, and background cycles.
4.  **Utility Functions**: Functions to manage the sliding window state for background transitions.

----------

## **Header File: `constants.h`**

### **Included Libraries**

-   **SDL2/SDL.h**: For rendering and texture manipulation.
-   **math.h**: For mathematical operations.
-   **stdbool.h**: For boolean data types.

### **Struct Definitions**

1.  **`Point`**:
    
    -   Used for 3D-to-2D projections.
    -   Contains:
        -   `world`: World coordinates (`x`, `y`, `z`).
        -   `screen`: Screen-space coordinates (`x`, `y`, `w`).
        -   `camera`: Camera-relative coordinates (`x`, `y`, `z`).
        -   `scale`: Perspective scaling factor.
2.  **`Color`**:
    
    -   Represents RGBA values for colors.
    -   Fields:
        -   `r`, `g`, `b`, `a` (red, green, blue, alpha).
3.  **`ColorScheme`**:
    
    -   Represents the color scheme for road, grass, rumble strips, and lane markers.
4.  **`Sprite`**:
    
    -   Represents a rectangular region in a texture (sprite sheet).
    -   Fields:
        -   `x`, `y`: Top-left corner.
        -   `w`, `h`: Width and height.
5.  **`Offset`**:
    
    -   Represents the position offset of a texture.
    -   Fields:
        -   `x`, `y`.
6.  **`LODRegion`**:
    
    -   Represents a region of a texture used for different Levels of Detail (LOD).
    -   Fields:
        -   `x`, `y`, `w`, `h`.
7.  **`BackgroundLayer`**:
    
    -   Represents LOD levels for a background layer.
    -   Fields:
        -   `highLOD`, `midLOD`, `lowLOD`: `SDL_Rect` regions for LOD textures.
8.  **`BackgroundCycleState`**:
    
    -   Represents the textures and offsets for a background cycle state.
    -   Fields:
        -   `L2`, `L3`, `L4`: Layer textures for low, mid, and high LOD.
        -   `L2Offset`, `L3Offset`, `L4Offset`: Offsets for these layers.
9.  **`SlidingWindow`**:
    
    -   Manages the background transition state.
    -   Fields:
        -   `currentStateIndex`: Current cycle state.
        -   `targetStateIndex`: Target cycle state.
        -   `progress`: Transition progress (0.0–1.0).
        -   `transitioning`: Whether a transition is in progress.
        -   `movingForward`: Direction of transition.

### **Constants**

1.  **Game Configuration**:
    
    -   `FPS`: Frames per second (60).
    -   `STEP`: Time step per frame.
    -   `WIDTH`, `HEIGHT`: Screen dimensions.
    -   `ROAD_WIDTH`: Width of the road in world coordinates.
    -   `SEGMENT_LENGTH`: Length of a road segment.
    -   `RUMBLE_LENGTH`: Length of alternating rumble strips.
    -   `LANE_COUNT`: Number of lanes.
    -   `FIELD_OF_VIEW`: Camera field of view.
    -   `CAMERA_HEIGHT`: Camera height above the road.
    -   `DRAW_DISTANCE`: Number of road segments rendered ahead.
    -   `FOG_DENSITY`: Fog density for distant segments.
2.  **Background Configuration**:
    
    -   `BACKGROUND_CYCLE_STATES_COUNT`: Number of background cycle states.
    -   `GAP_SIZE`: Gap between textures (in pixels).
    -   `SLIDE_SPEED`: Speed of background transitions.
3.  **Physics**:
    
    -   Speed and acceleration constants for the player:
        -   `MAX_SPEED`, `ACCELERATION`, `BRAKING`, `DECELERATION`, `OFF_ROAD_DECELERATION`, `OFF_ROAD_LIMIT`.
4.  **Sprite Scaling**:
    
    -   `SPRITE_SCALE`: Scale factor for sprites, derived from player sprite size.
5.  **Parallax Factors**:
    
    -   Defines relative speeds of background layers:
        -   `HILLS_PARALLAX_FACTOR`, `TREES_PARALLAX_FACTOR`, `MOUNTAINS_PARALLAX_FACTOR`.
6.  **Input Keys**:
    
    -   Key codes for player input (e.g., `SDLK_LEFT`, `SDLK_UP`).
7.  **Blend Factors**:
    
    -   Determines blending intensity for background LOD levels:
        -   `MID_LOD_BLEND_FACTOR`, `HIGH_LOD_BLEND_FACTOR`.

### **Extern Variables**

-   **Colors**: Predefined colors (`COLOR_SKY`, `COLOR_TREE`, `COLOR_FOG`).
-   **Color Schemes**: Predefined schemes for road sections (`COLOR_LIGHT`, `COLOR_DARK`, etc.).
-   **Background Layers**: LOD regions for each background layer (e.g., `BACKGROUND_TREES`, `BACKGROUND_HILLS`).
-   **Offsets**: Predefined offsets for LOD layers.
-   **Sprites**: Predefined sprite definitions for game elements.

----------

## **Source File: `constants.c`**

### **Purpose**

Defines the values of the global variables declared in `constants.h`. Implements utility functions for managing the sliding window.

### **Global Variables**

1.  **Colors**: Defined values for `COLOR_SKY`, `COLOR_TREE`, and `COLOR_FOG`.
2.  **Color Schemes**: Initialization of `COLOR_LIGHT`, `COLOR_DARK`, `COLOR_START`, and `COLOR_FINISH`.
3.  **Background Layers**: LOD definitions for trees, hills, mountains, and sky.
4.  **Offsets**: Predefined offsets for background layers.
5.  **Background Cycle States**: Predefined arrangements for each background phase.
6.  **Sprites**: Texture regions for billboards, trees, cars, and the player.

### **Utility Functions**

1.  **`hexToColor`**:
    
    -   Converts a hexadecimal color string (e.g., `"#FFFFFF"`) to a `Color` structure.
    -   Parameters:
        -   `hexString`: Hexadecimal color string.
    -   Returns:
        -   `Color`: RGBA representation.
2.  **`createSlidingWindow`**:
    
    -   Allocates and initializes a `SlidingWindow` structure.
    -   Returns:
        -   Pointer to the initialized `SlidingWindow`.
3.  **`startTransition`**:
    
    -   Begins a background transition.
    -   Parameters:
        -   `window`: Pointer to a `SlidingWindow`.
        -   `forward`: Direction of transition (`true` for forward).
    -   Updates `targetStateIndex` and sets `transitioning` to `true`.
4.  **`finalizeTransition`**:
    
    -   Completes a transition and updates the `currentStateIndex`.
    -   Parameters:
        -   `window`: Pointer to a `SlidingWindow`.

----------

## **Usage**

-   **Game Initialization**: Use the constants and color schemes to set up initial game states.
-   **Rendering**: Utilize background layers, parallax factors, and sprite definitions for efficient rendering.
-   **Transitions**: Manage smooth background transitions using the `SlidingWindow` utility functions.
##
`util.h` and `util.c`
## **Overview**

The `util.h` and `util.c` files provide utility functions and helpers for mathematical calculations, interpolation, fog effects, texture manipulation, and 3D-to-2D projections. These functions are essential for smooth gameplay mechanics, rendering, and visual effects in the game.

----------

## **Header File: `util.h`**

### **Included Libraries**

-   **SDL2/SDL.h**: For surface and texture manipulation.
-   **constants.h**: Provides access to common game constants.

### **Function Declarations**

#### **Mathematical Utilities**

1.  **`double timestamp()`**
    
    -   Returns the current time in seconds since the program started.
2.  **`double limit(double value, double min, double max)`**
    
    -   Constrains a value within the specified range `[min, max]`.
3.  **`double accelerate(double v, double accel, double dt)`**
    
    -   Calculates the new velocity after applying acceleration over time.
4.  **`double interpolate(double a, double b, double percent)`**
    
    -   Linearly interpolates between two values by a percentage.
5.  **`double easeIn(double a, double b, double percent)`**
    
    -   Smoothly starts interpolation using quadratic easing.
6.  **`double easeOut(double a, double b, double percent)`**
    
    -   Smoothly ends interpolation using quadratic easing.
7.  **`double easeInOut(double a, double b, double percent)`**
    
    -   Combines easing in and out for smooth transitions.
8.  **`double exponentialFog(double distance, double density)`**
    
    -   Computes fog intensity based on distance and density using an exponential formula.
9.  **`double increase(double start, double increment, double max)`**
    
    -   Increments a value and wraps it around if it exceeds a maximum value.

#### **3D-to-2D Projection**

10.  **`void project(Point* p, double cameraX, double cameraY, double cameraZ, double cameraDepth, int width, int height, double roadWidth)`**
    
    -   Projects a 3D world point onto a 2D screen space based on the camera's position and perspective.
11.  **`double percentRemaining(double n, double total)`**
    
    -   Returns the percentage of a value within a total.
12.  **`int overlap(double x1, double w1, double x2, double w2, double percent)`**
    
    -   Checks if two objects overlap based on their positions and widths.

#### **Linear Interpolation**

13.  **`float lerp_float(float a, float b, float t)`**
    -   Linearly interpolates between two floating-point values.

#### **Texture Manipulation**

14.  **`SDL_Surface* blend_surface_with_color(SDL_Surface* surface, SDL_Color blendColor, float blendFactor)`**
    
    -   Blends a surface with a specified color and blend factor.
15.  **`SDL_Texture* create_blended_texture(SDL_Renderer* renderer, SDL_Surface* sourceSurface, SDL_Rect srcRect, SDL_Color blendColor, float blendFactor)`**
    
    -   Creates a texture by blending a portion of a source surface with a color.

----------

## **Source File: `util.c`**

### **Purpose**

Implements the utility functions declared in `util.h` to provide mathematical, graphical, and rendering support.

----------

### **Major Functions**

#### **Mathematical Utilities**

1.  **`timestamp`**
    
    -   Returns the time since the game started, used for animations and timing.
2.  **`limit`**
    
    -   Ensures a value remains within specified bounds. Used for player position, speed, etc.
3.  **`accelerate`**
    
    -   Calculates the velocity change over time, considering acceleration or deceleration.
4.  **`interpolate`, `easeIn`, `easeOut`, `easeInOut`**
    
    -   Provide different methods of interpolation:
        -   **`interpolate`**: Linear interpolation for smooth transitions.
        -   **`easeIn`**, **`easeOut`**, **`easeInOut`**: Quadratic easing for smoother animations and transitions.
5.  **`exponentialFog`**
    
    -   Calculates the intensity of fog based on the player's distance, adding realism to distant objects.
6.  **`increase`**
    
    -   Wraps a value around a maximum limit, ensuring cyclic behavior (e.g., background scrolling).

----------

#### **3D-to-2D Projection**

7.  **`project`**
    -   Converts 3D coordinates into 2D screen space based on camera position, depth, and perspective.
    -   Essential for rendering road segments and objects in the pseudo-3D game world.

----------

#### **Collision and Percent Calculations**

8.  **`percentRemaining`**
    
    -   Determines the percentage of progress in a loop (e.g., player position on a track).
9.  **`overlap`**
    
    -   Checks whether two objects overlap in screen space.
    -   Used for collision detection between the player and cars or roadside sprites.
10.  **`lerp_float`**
    
    -   Performs linear interpolation for float values, useful for animations or gradual transitions.

----------

#### **Texture Manipulation**

11.  **`blend_surface_with_color`**
    
    -   Blends an entire surface with a specified color.
    -   Uses pixel manipulation to achieve a smooth color overlay.
    -   Applications:
        -   Dynamic background transitions.
        -   LOD effects for distant objects.
12.  **`create_blended_texture`**
    
    -   Extracts a portion of a surface (defined by `srcRect`) and blends it with a color.
    -   Converts the result into an SDL texture for rendering.
    -   Applications:
        -   Rendering background layers with varying levels of detail.
        -   Creating fog or fade effects dynamically.

----------

## **Key Features**

### **1. Interpolation and Easing**

-   Smooth animations and transitions are achieved using linear and quadratic interpolation functions.
-   Easing provides natural-looking accelerations and decelerations for movements.

### **2. Fog Effects**

-   **`exponentialFog`**: Simulates atmospheric depth by reducing visibility for distant objects.
-   Enhances the game's visual realism and difficulty perception.

### **3. 3D-to-2D Transformations**

-   The **`project`** function handles the core mechanics of pseudo-3D rendering.
-   Converts 3D world coordinates into 2D screen positions while applying perspective.

### **4. Texture Blending**

-   Functions like `blend_surface_with_color` and `create_blended_texture` support dynamic background transitions, color overlays, and advanced visual effects.

----------

## **Usage**

-   **Mathematics**:
    
    -   Use `interpolate` or `easeInOut` for smooth animations.
    -   Use `limit` and `increase` to constrain and cycle values.
    -   Use `accelerate` for velocity updates during game physics.
-   **Rendering**:
    
    -   Use `project` to calculate 2D positions for 3D objects.
    -   Use `blend_surface_with_color` and `create_blended_texture` for dynamic visual effects.
-   **Collision**:
    
    -   Use `overlap` to detect intersections between objects.
##
`game.h` and `game.c`
## **Overview**

The `game.h` and `game.c` files implement the core mechanics of the pseudo-3D racing game. They define the `Game` structure, manage game initialization, update the game state, render the visuals, handle user inputs, and manage resources.

----------

## **Header File: `game.h`**

### **Included Libraries**

-   **SDL2/SDL.h**: Rendering and event handling.
-   **SDL2/SDL_mixer.h**: Audio management.
-   **stdbool.h**: Boolean types.
-   **constants.h**: Access to game-wide constants.
-   **road.h**: Access to road-related structures and functions.

### **Struct Definitions**

1.  **`HUDItem`**:
    
    -   Represents a single HUD element.
    -   Fields:
        -   `value`: Numeric value displayed (e.g., speed or lap time).
        -   `texture`: Pre-rendered texture for display.
        -   `rect`: Position and size of the HUD item.
2.  **`HUD`**:
    
    -   Represents the complete HUD.
    -   Fields:
        -   `speed`: Displays current speed.
        -   `currentLapTime`: Displays the ongoing lap time.
        -   `lastLapTime`: Displays the last lap time.
        -   `fastLapTime`: Displays the fastest lap time.
3.  **`TransitionPhase`**:
    
    -   Enum representing transition states.
    -   Values:
        -   `TRANSITION_NONE`: No transition in progress.
        -   `TRANSITION_ACTIVE`: A transition is in progress.
4.  **`Game`**:
    
    -   Represents the overall game state.
    -   Fields include:
        -   **Rendering**:
            -   `renderer`: SDL renderer for rendering game visuals.
            -   `background`, `spritesheet`: Background and spritesheet textures.
        -   **Player State**:
            -   `position`: Player's position along the track.
            -   `speed`: Current speed.
            -   `playerX`: Horizontal offset from the road center.
            -   `playerZ`: Camera position along the Z-axis.
        -   **Physics**:
            -   `maxSpeed`, `accel`, `decel`: Maximum speed, acceleration, and deceleration.
            -   `centrifugal`: Centrifugal force for turns.
        -   **Track State**:
            -   `road`: Road structure managing segments and cars.
            -   `drawDistance`: Segments rendered ahead of the player.
        -   **Background**:
            -   `lodCycleThreshold`: Distance threshold for LOD cycling.
            -   `skyOffset`, `hillOffset`, `treeOffset`, `mountainOffset`: Horizontal offsets for scrolling layers.
            -   `parallaxEnabled`: Enables or disables parallax effects.
        -   **Transitions**:
            -   `transitionPhase`, `isTransitioning`: Indicates and manages transitions.
            -   `transitionProgress`, `transitionSpeed`: Manages transition progress.
        -   **HUD**:
            -   `hud`: Displays game information (speed, lap times, etc.).
        -   **Audio**:
            -   `music`: Background music.

### **Function Declarations**

1.  **`initGame(Game* game, SDL_Renderer* renderer)`**:
    -   Initializes the game, including textures, music, and track data.
2.  **`handleEvent(Game* game, SDL_Event* event)`**:
    -   Handles input events for player controls.
3.  **`updateGame(Game* game, double dt)`**:
    -   Updates the game state based on elapsed time.
4.  **`renderGame(Game* game)`**:
    -   Renders the game visuals to the screen.
5.  **`cleanupGame(Game* game)`**:
    -   Frees resources and cleans up the game.

----------

## **Source File: `game.c`**

### **Purpose**

Implements the game mechanics, including initialization, updating, rendering, and cleanup.

### **Major Functions**

#### **Initialization**

-   **`initGame(Game* game, SDL_Renderer* renderer)`**:
    -   Sets up SDL libraries (`TTF_Init`, `Mix_OpenAudio`).
    -   Loads textures (`background` and `spritesheet`).
    -   Loads and plays music.
    -   Initializes road, player physics, and HUD elements.
    -   Configures parallax factors and background scrolling.

#### **Input Handling**

-   **`handleEvent(Game* game, SDL_Event* event)`**:
    -   Processes keyboard events to set movement flags (`keyLeft`, `keyRight`, etc.).
    -   Handles key presses for acceleration, braking, and steering.

#### **Game Updates**

-   **`updateGame(Game* game, double dt)`**:
    
    -   Updates the player's position and speed based on inputs.
    -   Manages background transitions using a `SlidingWindow`.
    -   Handles collision detection with roadside objects and cars.
    -   Updates lap times and HUD elements.
    -   Applies parallax scrolling to background layers.
    -   Ensures player stays within road bounds and adjusts speed when off-road.
-   **`updateCars(Game* game, double dt, Segment* playerSegment, double playerW)`**:
    
    -   Updates AI cars' positions and collision avoidance logic.
    -   Moves cars between road segments as they progress.
-   **`updateCarOffset(Game* game, Car* car, Segment* carSegment, Segment* playerSegment, double playerW)`**:
    
    -   Adjusts the lateral position of AI cars to avoid collisions with the player and other cars.

#### **Rendering**

-   **`renderGame(Game* game)`**:
    -   Delegates rendering tasks to `render` and overlays the HUD.

#### **Cleanup**

-   **`cleanupGame(Game* game)`**:
    -   Frees textures, road segments, cars, music, and other resources.
    -   Shuts down SDL libraries (`Mix_CloseAudio`, `TTF_Quit`).

----------

## **Key Features**

1.  **LOD Cycling**:
    
    -   Implements background transitions using a sliding window mechanism for cycling between LOD states.
    -   Smoothly blends transitions with easing functions.
2.  **Parallax Scrolling**:
    
    -   Simulates depth by scrolling background layers at varying speeds relative to the player's position.
3.  **Collision Detection**:
    
    -   Detects collisions between the player and roadside sprites or AI cars.
    -   Adjusts player speed and position upon collision.
4.  **HUD Management**:
    
    -   Displays speed, current lap time, and other game metrics.
5.  **Audio Integration**:
    
    -   Plays background music using SDL_mixer.

----------

## **Usage**

-   **Initialization**: Call `initGame` to set up the game before entering the main loop.
-   **Input**: Use `handleEvent` to process player inputs.
-   **Update**: Call `updateGame` every frame to progress the game state.
-   **Rendering**: Use `renderGame` to draw the game visuals.
-   **Cleanup**: Call `cleanupGame` before exiting to free resources.
##
`road.h` and `road.c`
## **Overview**

The `road.h` and `road.c` files manage the game's road system, including the track layout, segments, sprites, cars, and features like hills, curves, and bumps. These files also handle procedural generation and dynamic reset of the track layout to create an engaging gameplay experience.

----------

## **Header File: `road.h`**

### **Included Libraries**

-   **`constants.h`**: Provides shared game constants like segment length and curve definitions.
-   **`SDL2/SDL.h`**: Used for rendering sprites and cars on the track.

### **Structures**

#### **SpriteInstance**

Represents a sprite placed on a road segment.

-   **`const Sprite* source`**: Reference to the sprite texture.
-   **`double offset`**: Horizontal offset from the center of the road.

#### **Car**

Represents an AI-controlled car on the track.

-   **`double offset`**: Horizontal position relative to the road.
-   **`double z`**: Position along the track's Z-axis.
-   **`const Sprite* sprite`**: Sprite for the car's appearance.
-   **`double speed`**: Speed of the car.
-   **`double percent`**: Position within the current segment.

#### **Segment**

Defines a road segment, which is a unit of the track.

-   **`int index`**: Segment index in the track.
-   **`double curve`**: Curvature of the segment.
-   **`Point p1, p2`**: Start and end points of the segment in 3D space.
-   **`ColorScheme color`**: Color scheme for the road and surroundings.
-   **`SpriteInstance* sprites`**: Array of sprites in the segment.
-   **`int spriteCount`**: Number of sprites in the segment.
-   **`Car** cars`**: Array of cars in the segment.
-   **`int carCount`**: Number of cars in the segment.
-   **`double clip`**: Clipping distance for rendering optimization.

#### **Road**

Manages the entire track layout.

-   **`Segment* segments`**: Array of all road segments.
-   **`int segmentCount`**: Number of segments in the road.
-   **`double trackLength`**: Total track length.
-   **`Car* cars`**: Array of all cars on the track.
-   **`int carCount`**: Number of cars on the track.

----------

### **Function Declarations**

#### **Road Construction**

1.  **`void buildRoad(Game* game)`**
    
    -   Initializes and builds the road layout.
2.  **`Segment* findSegment(Game* game, double position)`**
    
    -   Finds the segment corresponding to a specific position on the track.
3.  **`void resetRoad(Game* game)`**
    
    -   Resets the road to a predefined layout with features like curves and hills.
4.  **`void resetSprites(Game* game)`**
    
    -   Resets scenery sprites (e.g., trees, billboards) along the track.
5.  **`void resetCars(Game* game)`**
    
    -   Resets AI-controlled cars on the track.
6.  **`double lastY(Game* game)`**
    
    -   Returns the Y-coordinate of the last segment for smooth transitions.

----------

#### **Track Features**

7.  **`void addSegment(Game* game, double curve, double y)`**
    
    -   Adds a single road segment with specified curvature and elevation.
8.  **`void addSprite(Game* game, int segmentIndex, const Sprite* sprite, double offset)`**
    
    -   Adds a sprite to a specific segment of the road.
9.  **`void addRoad(Game* game, int enter, int hold, int leave, double curve, double y)`**
    
    -   Adds a sequence of segments to form a road section with curves and elevation changes.
10.  **Specialized Track Features**
    
    -   **`void addStraight(Game* game, int num)`**: Adds a straight road section.
    -   **`void addHill(Game* game, int num, double height)`**: Adds a hill.
    -   **`void addCurve(Game* game, int num, double curve, double height)`**: Adds a curved road section.
    -   **`void addLowRollingHills(Game* game, int num, double height)`**: Adds gentle rolling hills.
    -   **`void addSCurves(Game* game)`**: Adds a sequence of S-curves.
    -   **`void addBumps(Game* game)`**: Adds a bumpy road section.
    -   **`void addDownhillToEnd(Game* game, int num)`**: Adds a downhill section leading to the end of the track.

----------

## **Source File: `road.c`**

### **Purpose**

Implements the functions declared in `road.h`, handling procedural generation of road features and placement of sprites and cars.

----------

### **Major Functions**

#### **Road Construction**

1.  **`lastY`**
    
    -   Determines the Y-coordinate of the last segment, ensuring smooth elevation transitions when adding new segments.
2.  **`addSegment`**
    
    -   Allocates memory for a new segment and initializes its position, curvature, and color scheme.
3.  **`addRoad`**
    
    -   Adds multiple segments to create a road section with gradual transitions.
    -   Includes entry, hold, and exit phases for smooth curves and hills.
4.  **`findSegment`**
    
    -   Locates a segment based on a given Z-axis position, essential for collision detection and rendering.

----------

#### **Scenery and Cars**

5.  **`addSprite`**
    
    -   Places a sprite (e.g., a tree or billboard) on a specific road segment with a horizontal offset.
6.  **`resetSprites`**
    
    -   (Currently commented out) Example logic for placing sprites along the track.
7.  **`resetCars`**
    
    -   (Currently commented out) Example logic for populating the track with AI-controlled cars.

----------

#### **Track Features**

8.  **`addStraight`**
    
    -   Adds a straight section to the track. Useful for starting or resetting road alignment.
9.  **`addHill`**
    
    -   Creates a hill by modifying segment elevations.
10.  **`addCurve`**
    
    -   Adds a curved section of the road with optional elevation changes.
11.  **`addLowRollingHills`**
    
    -   Generates gentle rolling hills with alternating elevations.
12.  **`addSCurves`**
    
    -   Adds an S-curve sequence with varying curvature and elevation.
13.  **`addBumps`**
    
    -   Creates a bumpy section with alternating heights.
14.  **`addDownhillToEnd`**
    
    -   Adds a downhill section to conclude the track.

----------

#### **Reset Functions**

15.  **`resetRoad`**
    -   Clears the existing road layout and constructs a predefined track with various features.
    -   Sets start and finish lines for the race.

----------

## **Key Features**

### **1. Procedural Road Generation**

-   Functions like `addRoad`, `addHill`, and `addCurve` dynamically create road sections with varying curvature and elevation.

### **2. Modular Design**

-   Each segment is self-contained, allowing easy modification of road features.

### **3. Scenery Management**

-   The `addSprite` function allows dynamic placement of roadside scenery like trees and billboards.

### **4. AI Cars**

-   The `resetCars` function generates cars with random positions, speeds, and sprites for varied gameplay.

### **5. Predefined Track Layout**

-   The `resetRoad` function builds a complete track with hills, curves, and other features, providing a structured racing experience.

----------

## **Usage**

-   **Initialization**: Call `buildRoad` or `resetRoad` to construct the track at the start of the game.
-   **Adding Features**: Use functions like `addStraight`, `addHill`, and `addCurve` to add specific road sections.
-   **Scenery and Cars**:
    -   Use `addSprite` to place scenery objects.
    -   Use `resetCars` to populate the track with AI-controlled cars.
    
##
`render.h` and `render.c`
## **Overview**

The `render.h` and `render.c` files handle the graphical rendering of the game's elements, including the background layers, road segments, roadside objects, AI cars, and the player's car. These files manage the projection of 3D coordinates onto the 2D screen, enabling the pseudo-3D effect of the racing game.

----------

## **Header File: `render.h`**

### **Included Libraries**

-   **`SDL2/SDL.h`**: For rendering textures and handling graphical elements.
-   **`game.h`**: To access game structures such as `Game` and `Segment`.
-   **`constants.h`**: For shared constants like road width and segment length.

### **Function Prototypes**

#### **Main Rendering Functions**

1.  **`void render(Game* game)`**
    
    -   The primary rendering function that orchestrates the rendering of all game elements.
2.  **`void renderBackground(SDL_Renderer* renderer, SDL_Texture* background, int screenWidth, int screenHeight, const SDL_Rect* layerRect, double finalOffset, const Offset* layerPositionOffset, bool wrapEnabled)`**
    
    -   Renders background layers (e.g., sky, hills, trees) with optional parallax and wrapping.

#### **Road Rendering**

3.  **`void renderSegment(SDL_Renderer* renderer, int width, int lanes, double x1, double y1, double w1, double x2, double y2, double w2, double fog, const ColorScheme* color)`**
    
    -   Renders individual road segments with rumble strips, road surface, and lane markers.
4.  **`double rumbleWidth(double projectedRoadWidth, int lanes)`**
    
    -   Calculates the width of rumble strips based on road and lane width.
5.  **`double laneMarkerWidth(double projectedRoadWidth, int lanes)`**
    
    -   Calculates the width of lane markers.

#### **Sprites and Player**

6.  **`void renderSprite(Game* game, SDL_Texture* spritesheet, const Sprite* sprite, double scale, double destX, double destY, double offsetX, double offsetY, double clipY)`**
    
    -   Renders a sprite with scaling, positioning, and optional clipping.
7.  **`void renderPlayer(Game* game, double speedPercent, double scale, double destX, double destY, double steer, double updown)`**
    
    -   Renders the player's car with steering and elevation adjustments.

#### **Polygon Rendering**

8.  **`void renderPolygon(SDL_Renderer* renderer, double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, const Color* color)`**
    -   Draws a filled quadrilateral polygon for road segments and other shapes.

#### **Fog Effect**

9.  **`void renderFog(SDL_Renderer* renderer, double x, double y, double width, double height, double fog)`**
    -   Renders a semi-transparent fog overlay for depth effects.

----------

## **Source File: `render.c`**

### **Purpose**

Implements the rendering logic defined in `render.h`, focusing on translating game elements into visible graphics on the screen.

----------

### **Major Functions**

#### **1. Main Render Function**

-   **`void render(Game* game)`**
    -   The core function that:
        1.  Clears the screen.
        2.  Renders background layers with parallax scrolling.
        3.  Projects and renders road segments in 3D space.
        4.  Draws roadside objects, AI cars, and the player's car.

#### **2. Background Rendering**

-   **`void renderBackground`**
    -   Handles parallax scrolling of background layers.
    -   Can render layers with or without horizontal wrapping.
    -   Supports transitions between different background states.

#### **3. Road Segment Rendering**

-   **`void renderSegment`**
    -   Renders a single road segment with:
        -   Grass on either side.
        -   Rumble strips and lane markers.
        -   Fog effect for depth.

#### **4. Sprite Rendering**

-   **`void renderSprite`**
    
    -   Projects and scales sprites (e.g., trees, billboards) based on their position on the road.
    -   Clips sprites that extend outside the visible screen.
-   **`void renderPlayer`**
    
    -   Renders the player's car at the center of the screen, adjusting for steering and elevation changes.

#### **5. Polygon Rendering**

-   **`void renderPolygon`**
    -   Draws filled polygons using SDL's `SDL_RenderGeometry` function.
    -   Used for rendering road surfaces, lane markers, and rumble strips.

#### **6. Fog Effect**

-   **`void renderFog`**
    -   Applies a semi-transparent fog overlay to create a sense of depth.
    -   The fog density increases with distance.

#### **7. Utility Functions**

-   **`double rumbleWidth`**
    -   Computes the width of rumble strips based on the projected road width and number of lanes.
-   **`double laneMarkerWidth`**
    -   Computes the width of lane markers.

----------

## **Key Features**

### **1. Background Rendering with Parallax**

-   Background layers (e.g., sky, hills, trees) scroll at different speeds to create a parallax effect.
-   Supports smooth transitions between background cycles.

### **2. Pseudo-3D Road Rendering**

-   Projects 3D road segments onto a 2D plane using perspective calculations.
-   Dynamically adjusts road width and curvature based on the player's position and camera settings.

### **3. Dynamic Fog Effects**

-   Exponential fog density creates depth by gradually fading distant segments.
-   Fog color and intensity are customizable.

### **4. Player and Sprite Rendering**

-   Scales and positions sprites dynamically based on perspective.
-   Applies steering and elevation effects to the player's car.

### **5. Layer Transitions**

-   Smooth transitions between different background states using eased progress.

----------

## **Usage**

-   **Initialization**: Call `render` within the game loop to draw all game elements.
-   **Customization**:
    -   Adjust parallax factors for different background layers.
    -   Modify road and sprite colors in `constants.h` to change the visual theme.
-   **Optimization**:
    -   Use fog effects to reduce the detail of distant segments.
    -   Clip off-screen sprites to improve performance.
