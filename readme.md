# Pseudo 3D Racing Game

**Author:** Edvinas Butkus
**Registration number:** 2202906

## Description

This project is a prototype of a pseudo-3D racing game developed in C using the SDL2 library and its extensions. It draws inspiration from classic arcade racers like SEGA's Out Run (1986), aiming to recreate their visual style and core gameplay mechanics within a modern development environment. The game features a segment-based road engine capable of rendering curves and elevation changes, a dynamic multi-layered parallax scrolling background with visual transitions triggered by player progress, AI opponents with collision detection and aggressive ramming behaviors, and a level loading system that parses track layouts and resources from JSON files.

## Features

* **Pseudo-3D Rendering:** Segment-based road projection with curves and elevation. Depth-based sprite scaling for vehicles and objects.
* **Dynamic Background:** Multi-layered parallax scrolling background that adapts to road elevation. Sliding window system for smooth transitions between background themes.
* **Gameplay Mechanics:** Responsive player controls (keyboard), basic vehicle physics with off-road penalties, lap timing and race completion logic, Heads-Up Display (HUD), and an end-of-race leaderboard.
* **AI Opponents:** Multiple AI drivers navigate the track, adjust speed for curves, utilize catch-up mechanics, and exhibit varied behaviors including lane holding, wobbling, lane switching, and aggressive ramming. Includes AI recovery logic.
* **Collision System:** Detects and handles collisions between Player vs. AI, AI vs. AI, and vehicles vs. environment sprites (trees, billboards). Responses include lateral pushes and speed adjustments.
* **Level Loading:** Dynamically loads level layouts, resources (textures, music), background configurations, and track commands from external JSON files. Road geometry is generated at runtime based on level file commands.
* **User Interface:** In-game menus for starting, pausing, restarting, and exiting the game.

## Dependencies / Prerequisites

To compile and run this project, you will need:

* A C compiler (like GCC)
* SDL2 development library
* SDL2_image development library
* SDL2_ttf development library
* SDL2_mixer development library
* SDL2_gfx development library
* cJSON library (ensure headers and library file are accessible to the compiler/linker)

On Debian/Ubuntu-based systems, you can typically install the SDL dependencies like this:
`sudo apt-get update`
`sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-gfx-dev`

You will need to download or build the cJSON library separately. Ensure its header (`cJSON.h`) is in your include path.

## Compilation

Navigate to the directory containing the source code files (`*.c`, `*.h`) in your terminal. Run the following command:

```bash
gcc -g *.c -o game `sdl2-config --cflags --libs` -lSDL2_gfx -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lcjson -lm