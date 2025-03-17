// level_loader.h
#ifndef LEVEL_LOADER_H
#define LEVEL_LOADER_H

#include "cJSON.h"
#include "level.h"
#include "game.h"

// Function to load a level from a JSON file
bool loadLevel(Game* game, const char* levelFilePath);

// Helper function to load background data from JSON
bool loadBackground(cJSON* backgroundJSON, LevelBackground* levelBackground);

// Helper function to load road data from JSON
bool loadRoad(cJSON* roadJSON, LevelRoadData* roadData);

#endif // LEVEL_LOADER_H
