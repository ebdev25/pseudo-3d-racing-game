// level_loader.c
#include "level_loader.h"
#include "background.h"
#include "constants.h"
#include "paths.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include <stdalign.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

static const Sprite* resolveScenerySpriteName(const char* name) {
    if (!name) {
        return NULL;
    }
    if (SDL_strcasecmp(name, "palm_tree_left") == 0) {
        return &SPRITE_PALM_TREE_LEFT;
    }
    if (SDL_strcasecmp(name, "palm_tree_right") == 0) {
        return &SPRITE_PALM_TREE_RIGHT;
    }
    if (SDL_strcasecmp(name, "rock") == 0) {
        return &SPRITE_ROCK;
    }
    if (SDL_strcasecmp(name, "umbrella") == 0) {
        return &SPRITE_UMBRELLA;
    }
    if (SDL_strcasecmp(name, "billboard01") == 0 || SDL_strcasecmp(name, "billboard1") == 0) {
        return &SPRITE_BILLBOARD01;
    }
    if (SDL_strcasecmp(name, "billboard02") == 0 || SDL_strcasecmp(name, "billboard2") == 0) {
        return &SPRITE_BILLBOARD02;
    }
    if (SDL_strcasecmp(name, "billboard03") == 0 || SDL_strcasecmp(name, "billboard3") == 0) {
        return &SPRITE_BILLBOARD03;
    }
    if (SDL_strcasecmp(name, "billboard04") == 0 || SDL_strcasecmp(name, "billboard4") == 0) {
        return &SPRITE_BILLBOARD04;
    }
    if (SDL_strcasecmp(name, "billboard05") == 0 || SDL_strcasecmp(name, "billboard5") == 0) {
        return &SPRITE_BILLBOARD05;
    }
    if (SDL_strcasecmp(name, "billboard06") == 0 || SDL_strcasecmp(name, "billboard6") == 0) {
        return &SPRITE_BILLBOARD06;
    }
    if (SDL_strcasecmp(name, "billboard07") == 0 || SDL_strcasecmp(name, "billboard7") == 0) {
        return &SPRITE_BILLBOARD07;
    }
    if (SDL_strcasecmp(name, "billboard08") == 0 || SDL_strcasecmp(name, "billboard8") == 0) {
        return &SPRITE_BILLBOARD08;
    }
    if (SDL_strcasecmp(name, "billboard09") == 0 || SDL_strcasecmp(name, "billboard9") == 0) {
        return &SPRITE_BILLBOARD09;
    }
    return NULL;
}

static void clearLoadedScenery(Game* game) {
    game->loadedScenery.items = NULL;
    game->loadedScenery.itemCount = 0;
    game->loadedScenery.repeatRules = NULL;
    game->loadedScenery.repeatRuleCount = 0;
}

/*
 * instances[]: { "sprite": "<id>", "segment": <int>, "offset": <float> }
 * segment < 0 counts from end of track (e.g. -25 -> segmentCount - 25).
 */
static bool parseSceneryInstances(Game* game, cJSON* arr, LevelSceneryItem** outItems, int* outCount) {
    *outItems = NULL;
    *outCount = 0;
    if (!arr || !cJSON_IsArray(arr)) {
        return true;
    }
    int n = cJSON_GetArraySize(arr);
    if (n <= 0) {
        return true;
    }
    LevelSceneryItem* buf =
        arena_alloc(&game->levelArena, (size_t)n * sizeof(LevelSceneryItem), alignof(LevelSceneryItem));
    if (!buf) {
        SDL_Log("parseSceneryInstances: arena alloc failed");
        return false;
    }
    int w = 0;
    for (int i = 0; i < n; i++) {
        cJSON* o = cJSON_GetArrayItem(arr, i);
        if (!cJSON_IsObject(o)) {
            continue;
        }
        cJSON* sp = cJSON_GetObjectItemCaseSensitive(o, "sprite");
        if (!cJSON_IsString(sp) || sp->valuestring == NULL) {
            continue;
        }
        const Sprite* spr = resolveScenerySpriteName(sp->valuestring);
        if (!spr) {
            SDL_Log("scenery: unknown sprite \"%s\"", sp->valuestring);
            continue;
        }
        cJSON* seg = cJSON_GetObjectItemCaseSensitive(o, "segment");
        if (!cJSON_IsNumber(seg)) {
            continue;
        }
        cJSON* off = cJSON_GetObjectItemCaseSensitive(o, "offset");
        double offv = cJSON_IsNumber(off) ? off->valuedouble : 0.0;
        buf[w].sprite = spr;
        buf[w].segmentIndex = (int)seg->valuedouble;
        buf[w].offset = offv;
        w++;
    }
    *outItems = buf;
    *outCount = w;
    return true;
}

/*
 * repeatAlong[]: { "from": int, "to": int, "step": int, "placements": [ { "sprite", "offset" }, ... ] }
 * from/to < 0 count from end. "to" is exclusive (segments n with from <= n < to).
 */
static bool parseSceneryRepeatAlong(Game* game, cJSON* arr) {
    if (!arr || !cJSON_IsArray(arr)) {
        return true;
    }
    int n = cJSON_GetArraySize(arr);
    if (n <= 0) {
        return true;
    }
    LevelSceneryRepeatRule* rules =
        arena_alloc(&game->levelArena, (size_t)n * sizeof(LevelSceneryRepeatRule), alignof(LevelSceneryRepeatRule));
    if (!rules) {
        SDL_Log("parseSceneryRepeatAlong: arena alloc failed");
        return false;
    }
    memset(rules, 0, (size_t)n * sizeof(LevelSceneryRepeatRule));
    int ruleWrite = 0;
    for (int i = 0; i < n; i++) {
        cJSON* o = cJSON_GetArrayItem(arr, i);
        if (!cJSON_IsObject(o)) {
            continue;
        }
        cJSON* from = cJSON_GetObjectItemCaseSensitive(o, "from");
        cJSON* to = cJSON_GetObjectItemCaseSensitive(o, "to");
        cJSON* stepJ = cJSON_GetObjectItemCaseSensitive(o, "step");
        cJSON* placements = cJSON_GetObjectItemCaseSensitive(o, "placements");
        if (!cJSON_IsNumber(from) || !cJSON_IsNumber(to) || !cJSON_IsArray(placements)) {
            continue;
        }
        int stepv = cJSON_IsNumber(stepJ) ? (int)stepJ->valuedouble : 1;
        if (stepv <= 0) {
            stepv = 1;
        }
        int pc = cJSON_GetArraySize(placements);
        if (pc <= 0) {
            continue;
        }
        LevelSceneryRepeatPlacement* slots = arena_alloc(
            &game->levelArena, (size_t)pc * sizeof(LevelSceneryRepeatPlacement), alignof(LevelSceneryRepeatPlacement));
        if (!slots) {
            SDL_Log("parseSceneryRepeatAlong: placement arena alloc failed");
            return false;
        }
        int pw = 0;
        for (int p = 0; p < pc; p++) {
            cJSON* po = cJSON_GetArrayItem(placements, p);
            if (!cJSON_IsObject(po)) {
                continue;
            }
            cJSON* sp = cJSON_GetObjectItemCaseSensitive(po, "sprite");
            if (!cJSON_IsString(sp) || sp->valuestring == NULL) {
                continue;
            }
            const Sprite* spr = resolveScenerySpriteName(sp->valuestring);
            if (!spr) {
                SDL_Log("scenery repeat: unknown sprite \"%s\"", sp->valuestring);
                continue;
            }
            cJSON* off = cJSON_GetObjectItemCaseSensitive(po, "offset");
            double offv = cJSON_IsNumber(off) ? off->valuedouble : 0.0;
            slots[pw].sprite = spr;
            slots[pw].offset = offv;
            pw++;
        }
        if (pw <= 0) {
            continue;
        }
        rules[ruleWrite].fromSegment = (int)from->valuedouble;
        rules[ruleWrite].toSegment = (int)to->valuedouble;
        rules[ruleWrite].step = stepv;
        rules[ruleWrite].placements = slots;
        rules[ruleWrite].placementCount = pw;
        ruleWrite++;
    }
    game->loadedScenery.repeatRules = rules;
    game->loadedScenery.repeatRuleCount = ruleWrite;
    return true;
}

/*
 * "scenery": [ instances... ]  OR  { "instances": [...], "repeatAlong": [...] }
 */
static bool loadSceneryFromJson(Game* game, cJSON* sceneryJSON) {
    clearLoadedScenery(game);
    if (!sceneryJSON || cJSON_IsNull(sceneryJSON)) {
        return true;
    }
    if (cJSON_IsArray(sceneryJSON)) {
        return parseSceneryInstances(game, sceneryJSON, &game->loadedScenery.items, &game->loadedScenery.itemCount);
    }
    if (!cJSON_IsObject(sceneryJSON)) {
        SDL_Log("loadSceneryFromJson: ignoring 'scenery' (expected object or array)");
        clearLoadedScenery(game);
        return true;
    }
    cJSON* inst = cJSON_GetObjectItemCaseSensitive(sceneryJSON, "instances");
    if (!parseSceneryInstances(game, inst, &game->loadedScenery.items, &game->loadedScenery.itemCount)) {
        return false;
    }
    cJSON* rep = cJSON_GetObjectItemCaseSensitive(sceneryJSON, "repeatAlong");
    if (!parseSceneryRepeatAlong(game, rep)) {
        return false;
    }
    return true;
}

// Helper to parse hex colors like "#RRGGBB"
static Color hexToColor(const char* hexString) {
    Color color = {0, 0, 0, 255};
    if (hexString && *hexString == '#') {
        unsigned int r, g, b;
        if (sscanf(hexString + 1, "%02x%02x%02x", &r, &g, &b) == 3) {
            color.r = (Uint8)r;
            color.g = (Uint8)g;
            color.b = (Uint8)b;
        }
    }
    return color;
}

// Function to load background from JSON with thorough debug logs
bool loadBackground(Game* game, cJSON* backgroundJSON, LevelBackground* levelBackground) 
{
    //SDL_Log("loadBackground: Entered function.\n");

    if (!cJSON_IsObject(backgroundJSON)) {
        //SDL_Log("loadBackground: 'background' is not an object.\n");
        return false;
    }
    //SDL_Log("loadBackground: 'background' is a valid object.\n");

    // Obtain the 'cycleStates' array
    cJSON* cycleStatesJSON = cJSON_GetObjectItemCaseSensitive(backgroundJSON, "cycleStates");
    if (!cJSON_IsArray(cycleStatesJSON)) {
        //SDL_Log("loadBackground: 'background.cycleStates' is missing or not an array.\n");
        return false;
    }
    //SDL_Log("loadBackground: Found 'cycleStates' array.\n");

    // Count the number of states
    int cycleStateCount = cJSON_GetArraySize(cycleStatesJSON);
    //SDL_Log("loadBackground: cycleStateCount = %d\n", cycleStateCount);

    if (cycleStateCount <= 0) {
        //SDL_Log("loadBackground: No cycleStates found in 'background'.\n");
        return false;
    }

    // Allocate memory for the cycleStates array
    levelBackground->cycleStateCount = cycleStateCount;
    levelBackground->cycleStates = arena_alloc(&game->levelArena, sizeof(BackgroundCycleState) * cycleStateCount, alignof(BackgroundCycleState));
    if (!levelBackground->cycleStates) {
        //SDL_Log("loadBackground: Failed to allocate memory for cycleStates.\n");
        return false;
    }
    //SDL_Log("loadBackground: Successfully allocated memory for %d cycleStates.\n", cycleStateCount);

    // Parse each cycle state in the array
    for (int i = 0; i < cycleStateCount; i++) {
        //SDL_Log("loadBackground: Parsing cycleStates[%d].\n", i);

        cJSON* stateJSON = cJSON_GetArrayItem(cycleStatesJSON, i);
        if (!cJSON_IsObject(stateJSON)) {
            //SDL_Log("loadBackground: cycleStates[%d] is not an object.\n", i);
            continue; // Skip to the next state but do not return false
        }

        BackgroundCycleState* state = &levelBackground->cycleStates[i];
        memset(state, 0, sizeof(BackgroundCycleState)); // Initialize to avoid junk data

        // 1) Extract 'name'
        cJSON* nameJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "name");
        if (cJSON_IsString(nameJSON) && (nameJSON->valuestring != NULL)) {
            state->name = arena_strdup(&game->levelArena, nameJSON->valuestring);
            //SDL_Log("loadBackground: cycleStates[%d].name = '%s'\n", i, state->name);
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].name is missing or not a valid string.\n", i);
            state->name = arena_strdup(&game->levelArena, "Unnamed Biome");
        }

        // 2) Extract 'textureRects'
        cJSON* textureRectsJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "textureRects");
        if (cJSON_IsArray(textureRectsJSON) && cJSON_GetArraySize(textureRectsJSON) == 3) {
            //SDL_Log("loadBackground: cycleStates[%d] has a valid 'textureRects' array.\n", i);

            for (int j = 0; j < 3; j++) {
                cJSON* rectJSON = cJSON_GetArrayItem(textureRectsJSON, j);
                if (cJSON_IsArray(rectJSON) && cJSON_GetArraySize(rectJSON) == 4) {
                    cJSON* x = cJSON_GetArrayItem(rectJSON, 0);
                    cJSON* y = cJSON_GetArrayItem(rectJSON, 1);
                    cJSON* w = cJSON_GetArrayItem(rectJSON, 2);
                    cJSON* h = cJSON_GetArrayItem(rectJSON, 3);
                    if (cJSON_IsNumber(x) && cJSON_IsNumber(y) && cJSON_IsNumber(w) && cJSON_IsNumber(h)) {
                        state->textureRects[j].x = x->valueint;
                        state->textureRects[j].y = y->valueint;
                        state->textureRects[j].w = w->valueint;
                        state->textureRects[j].h = h->valueint;
                        //SDL_Log("loadBackground: cycleStates[%d].textureRects[%d] = (%d,%d,%d,%d)\n",
                        //    i, j, state->textureRects[j].x, state->textureRects[j].y,
                        //    state->textureRects[j].w, state->textureRects[j].h);
                    } else {
                        //SDL_Log("loadBackground: cycleStates[%d].textureRects[%d] contains non-numeric values.\n", i, j);
                        state->textureRects[j] = (SDL_Rect){0, 0, 0, 0};
                    }
                } else {
                    //SDL_Log("loadBackground: cycleStates[%d].textureRects[%d] is invalid (not an array of 4 ints).\n", i, j);
                    state->textureRects[j] = (SDL_Rect){0, 0, 0, 0};
                }
            }
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].textureRects is missing or not an array of size 3.\n", i);
            memset(state->textureRects, 0, sizeof(state->textureRects));
        }

        // 3) Extract 'offsets'
        cJSON* offsetsJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "offsets");
        if (cJSON_IsArray(offsetsJSON) && cJSON_GetArraySize(offsetsJSON) == 3) {
            //SDL_Log("loadBackground: cycleStates[%d] has a valid 'offsets' array.\n", i);

            for (int j = 0; j < 3; j++) {
                cJSON* offsetJSON = cJSON_GetArrayItem(offsetsJSON, j);
                if (cJSON_IsObject(offsetJSON)) {
                    cJSON* xJSON = cJSON_GetObjectItemCaseSensitive(offsetJSON, "x");
                    cJSON* yJSON = cJSON_GetObjectItemCaseSensitive(offsetJSON, "y");

                    state->offsets[j].x = cJSON_IsNumber(xJSON) ? xJSON->valueint : 0;
                    state->offsets[j].y = cJSON_IsNumber(yJSON) ? yJSON->valueint : 0;
                    //SDL_Log("loadBackground: cycleStates[%d].offsets[%d] = x:%d, y:%d\n",
                    //    i, j, state->offsets[j].x, state->offsets[j].y);
                } else {
                    //SDL_Log("loadBackground: cycleStates[%d].offsets[%d] is invalid (not an object).\n", i, j);
                    state->offsets[j].x = 0;
                    state->offsets[j].y = 0;
                }
            }
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].offsets is missing or not an array of size 3.\n", i);
            memset(state->offsets, 0, sizeof(state->offsets));
        }

        // 4) Extract 'speeds'
        cJSON* speedsJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "speeds");
        if (cJSON_IsArray(speedsJSON) && cJSON_GetArraySize(speedsJSON) == 3) {
            //SDL_Log("loadBackground: cycleStates[%d] has valid 'speeds'.\n", i);

            for (int j = 0; j < 3; j++) {
                cJSON* speedJSON = cJSON_GetArrayItem(speedsJSON, j);
                if (cJSON_IsNumber(speedJSON)) {
                    state->speeds[j] = (float)speedJSON->valuedouble;
                    //SDL_Log("loadBackground: cycleStates[%d].speeds[%d] = %f\n", i, j, state->speeds[j]);
                } else {
                    //SDL_Log("loadBackground: cycleStates[%d].speeds[%d] is not a number.\n", i, j);
                    state->speeds[j] = 0.0f;
                }
            }
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].speeds is missing or not an array of size 3.\n", i);
            memset(state->speeds, 0, sizeof(state->speeds));
        }

        // 5) Extract 'verticalSpeeds'
        cJSON* verticalSpeedsJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "verticalSpeeds");
        if (cJSON_IsArray(verticalSpeedsJSON) && cJSON_GetArraySize(verticalSpeedsJSON) == 3) {
            //SDL_Log("loadBackground: cycleStates[%d] has valid 'verticalSpeeds'.\n", i);

            for (int j = 0; j < 3; j++) {
                cJSON* vSpeedJSON = cJSON_GetArrayItem(verticalSpeedsJSON, j);
                if (cJSON_IsNumber(vSpeedJSON)) {
                    state->verticalSpeeds[j] = (float)vSpeedJSON->valuedouble;
                    //SDL_Log("loadBackground: cycleStates[%d].verticalSpeeds[%d] = %f\n", i, j, state->verticalSpeeds[j]);
                } else {
                    //SDL_Log("loadBackground: cycleStates[%d].verticalSpeeds[%d] is not a number.\n", i, j);
                    state->verticalSpeeds[j] = 0.0f;
                }
            }
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].verticalSpeeds is missing or not an array of size 3.\n", i);
            memset(state->verticalSpeeds, 0, sizeof(state->verticalSpeeds));
        }

        // 6) Extract 'layerTransitionSpeeds'
        cJSON* layerTransitionSpeedsJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "layerTransitionSpeeds");
        if (cJSON_IsArray(layerTransitionSpeedsJSON) && cJSON_GetArraySize(layerTransitionSpeedsJSON) == 3) {
            //SDL_Log("loadBackground: cycleStates[%d] has valid 'layerTransitionSpeeds'.\n", i);

            for (int j = 0; j < 3; j++) {
                cJSON* ltSpeedJSON = cJSON_GetArrayItem(layerTransitionSpeedsJSON, j);
                if (cJSON_IsNumber(ltSpeedJSON)) {
                    state->layerTransitionSpeeds[j] = (float)ltSpeedJSON->valuedouble;
                    //SDL_Log("loadBackground: cycleStates[%d].layerTransitionSpeeds[%d] = %f\n", i, j, state->layerTransitionSpeeds[j]);
                } else {
                    //SDL_Log("loadBackground: cycleStates[%d].layerTransitionSpeeds[%d] is not a number.\n", i, j);
                    state->layerTransitionSpeeds[j] = 0.0f;
                }
            }
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].layerTransitionSpeeds is missing or not an array of size 3.\n", i);
            memset(state->layerTransitionSpeeds, 0, sizeof(state->layerTransitionSpeeds));
        }

        // 7) Extract 'overlapMargins'
        cJSON* overlapMarginsJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "overlapMargins");
        if (cJSON_IsArray(overlapMarginsJSON) && cJSON_GetArraySize(overlapMarginsJSON) == 3) {
            //SDL_Log("loadBackground: cycleStates[%d] has valid 'overlapMargins'.\n", i);

            for (int j = 0; j < 3; j++) {
                cJSON* marginJSON = cJSON_GetArrayItem(overlapMarginsJSON, j);
                if (cJSON_IsNumber(marginJSON)) {
                    state->overlapMargins[j] = (float)marginJSON->valuedouble;
                    //SDL_Log("loadBackground: cycleStates[%d].overlapMargins[%d] = %f\n", i, j, state->overlapMargins[j]);
                } else {
                    //SDL_Log("loadBackground: cycleStates[%d].overlapMargins[%d] is not a number.\n", i, j);
                    state->overlapMargins[j] = 0.0f;
                }
            }
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].overlapMargins is missing or not an array of size 3.\n", i);
            memset(state->overlapMargins, 0, sizeof(state->overlapMargins));
        }

        // 8) Extract 'skyColor'
        cJSON* skyColorJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "skyColor");
        if (cJSON_IsArray(skyColorJSON) && cJSON_GetArraySize(skyColorJSON) == 4) {
            cJSON* sr = cJSON_GetArrayItem(skyColorJSON, 0);
            cJSON* sg = cJSON_GetArrayItem(skyColorJSON, 1);
            cJSON* sb = cJSON_GetArrayItem(skyColorJSON, 2);
            cJSON* sa = cJSON_GetArrayItem(skyColorJSON, 3);

            if (cJSON_IsNumber(sr) && cJSON_IsNumber(sg) && cJSON_IsNumber(sb) && cJSON_IsNumber(sa)) {
                state->skyColor = (BackgroundColor){
                    (Uint8)sr->valueint,
                    (Uint8)sg->valueint,
                    (Uint8)sb->valueint,
                    (Uint8)sa->valueint
                };
                //SDL_Log("loadBackground: cycleStates[%d].skyColor = (%d,%d,%d,%d)\n",
                //    i, state->skyColor.r, state->skyColor.g, state->skyColor.b, state->skyColor.a);
            } else {
                //SDL_Log("loadBackground: cycleStates[%d].skyColor array elements are not all numbers.\n", i);
                state->skyColor = (BackgroundColor){0, 0, 0, 255};
            }
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].skyColor is missing or invalid.\n", i);
            state->skyColor = (BackgroundColor){0, 0, 0, 255};
        }

        // 9) Extract 'roadColor'
        cJSON* roadColorJSON = cJSON_GetObjectItemCaseSensitive(stateJSON, "roadColor");
        if (cJSON_IsArray(roadColorJSON) && cJSON_GetArraySize(roadColorJSON) == 4) {
            cJSON* rr = cJSON_GetArrayItem(roadColorJSON, 0);
            cJSON* rg = cJSON_GetArrayItem(roadColorJSON, 1);
            cJSON* rb = cJSON_GetArrayItem(roadColorJSON, 2);
            cJSON* ra = cJSON_GetArrayItem(roadColorJSON, 3);

            if (cJSON_IsNumber(rr) && cJSON_IsNumber(rg) && cJSON_IsNumber(rb) && cJSON_IsNumber(ra)) {
                state->roadColor = (BackgroundColor){
                    (Uint8)rr->valueint,
                    (Uint8)rg->valueint,
                    (Uint8)rb->valueint,
                    (Uint8)ra->valueint
                };
                //SDL_Log("loadBackground: cycleStates[%d].roadColor = (%d,%d,%d,%d)\n",
                //    i, state->roadColor.r, state->roadColor.g, state->roadColor.b, state->roadColor.a);
            } else {
                //SDL_Log("loadBackground: cycleStates[%d].roadColor array elements are not all numbers.\n", i);
                state->roadColor = (BackgroundColor){0, 0, 0, 255};
            }
        } else {
            //SDL_Log("loadBackground: cycleStates[%d].roadColor is missing or invalid.\n", i);
            state->roadColor = (BackgroundColor){0, 0, 0, 255};
        }

        // Initialize extraSlice
        for (int j = 0; j < 3; j++) {
            state->extraSlice[j] = 0;
        }

        // Texture pointer remains NULL until resource loading (IMG_LoadTexture).
        state->texture = NULL; 
        //SDL_Log("loadBackground: Finished parsing cycleStates[%d].\n", i);
    }

    //SDL_Log("loadBackground: Finished parsing all cycleStates successfully.\n");
    return true;
}

bool loadRoad(Game* game, cJSON* roadJSON, LevelRoadData* roadData)
{
    if (!cJSON_IsObject(roadJSON)) {
        SDL_Log("loadRoad: 'road' is not an object.\n");
        return false;
    }

    cJSON* commandsJSON = cJSON_GetObjectItemCaseSensitive(roadJSON, "commands");
    if (!cJSON_IsArray(commandsJSON)) {
        SDL_Log("loadRoad: 'road.commands' is missing or not an array.\n");
        return false;
    }

    int cmdCount = cJSON_GetArraySize(commandsJSON);
    if (cmdCount <= 0) {
        SDL_Log("loadRoad: No commands found in 'road.commands'.\n");
        return false;
    }

    // Allocate memory for the commands
    roadData->commandCount = cmdCount;
    roadData->commands = arena_alloc(&game->levelArena, sizeof(RoadCommand) * cmdCount, alignof(RoadCommand));
    if (!roadData->commands) {
        SDL_Log("loadRoad: Failed to allocate memory for road commands.\n");
        return false;
    }
    memset(roadData->commands, 0, sizeof(RoadCommand) * cmdCount);

    // Parse each command
    for (int i = 0; i < cmdCount; i++) {
        cJSON* cmdJSON = cJSON_GetArrayItem(commandsJSON, i);
        if (!cJSON_IsObject(cmdJSON)) {
            SDL_Log("loadRoad: commands[%d] is not an object.\n", i);
            continue; // skip but do not fail
        }

        RoadCommand* cmd = &roadData->commands[i];
        memset(cmd, 0, sizeof(RoadCommand));

        // Read "type"
        cJSON* typeJSON = cJSON_GetObjectItemCaseSensitive(cmdJSON, "type");
        if (cJSON_IsString(typeJSON) && typeJSON->valuestring != NULL) {
            cmd->type = arena_strdup(&game->levelArena, typeJSON->valuestring);
        } else {
            SDL_Log("loadRoad: commands[%d].type is missing or not a string. Using 'unknown'.\n", i);
            cmd->type = arena_strdup(&game->levelArena, "unknown");
        }

        // Read "params" array
        cJSON* paramsJSON = cJSON_GetObjectItemCaseSensitive(cmdJSON, "params");
        if (cJSON_IsArray(paramsJSON)) {
            int pCount = cJSON_GetArraySize(paramsJSON);
            if (pCount > 0) {
                cmd->params = arena_alloc(&game->levelArena, sizeof(float) * pCount, alignof(float));

                if (!cmd->params) {
                    SDL_Log("loadRoad: Failed to allocate params for command %d.\n", i);
                    cmd->paramCount = 0;
                } else {
                    memset(cmd->params, 0, sizeof(float) * pCount);
                    cmd->paramCount = pCount;

                    for (int j = 0; j < pCount; j++) {
                        cJSON* val = cJSON_GetArrayItem(paramsJSON, j);
                        if (cJSON_IsNumber(val)) {
                            cmd->params[j] = (float)val->valuedouble;
                        } else {
                            SDL_Log("loadRoad: commands[%d].params[%d] is not a number. Defaulting to 0.\n", i, j);
                            cmd->params[j] = 0.0f;
                        }
                    }
                }
            }
        } else {
            SDL_Log("loadRoad: commands[%d].params is missing or not an array.\n", i);
        }

        SDL_Log("loadRoad: Command[%d] => type='%s', paramCount=%d\n", 
                 i, cmd->type, cmd->paramCount);
    }

    SDL_Log("loadRoad: Successfully parsed %d road commands.\n", cmdCount);
    return true;
}

/* Tear down GPU resources and sliding window created during an in-progress loadLevel. */
static void rollbackLevelLoadGpuResources(Game* game) {
    game->currentCycleState = NULL;
    game->targetCycleState = NULL;
    if (game->slidingWindow) {
        freeBackgroundResources(game->slidingWindow);
        free(game->slidingWindow);
        game->slidingWindow = NULL;
    }
    if (game->music) {
        Mix_FreeMusic(game->music);
        game->music = NULL;
    }
    if (game->spritesheet) {
        SDL_DestroyTexture(game->spritesheet);
        game->spritesheet = NULL;
    }
    if (game->background) {
        SDL_DestroyTexture(game->background);
        game->background = NULL;
    }
}

// Function to load a level from a JSON file
bool loadLevel(Game* game, const char* levelFilePath)
{
    char resolved_level[1024];
    if (!paths_resolve(resolved_level, sizeof(resolved_level), levelFilePath)) {
        SDL_Log("loadLevel: level file not found: %s", levelFilePath);
        return false;
    }

    // 1. Read the entire file
    FILE* file = fopen(resolved_level, "rb");
    if (!file) {
        SDL_Log("loadLevel: fopen failed: %s", resolved_level);
        return false;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char* fileData = (char*)malloc(fileSize + 1);
    if (!fileData) {
        //SDL_Log("Memory allocation failed for level file data.\n");
        fclose(file);
        return false;
    }
    size_t readSize = fread(fileData, 1, fileSize, file);
    if (readSize != (size_t)fileSize) {
        //SDL_Log("Failed to read the entire level file.\n");
        free(fileData);
        fclose(file);
        return false;
    }
    fileData[fileSize] = '\0';
    fclose(file);

    //SDL_Log("File contents read:\n%s\n", fileData);

    // 2. Parse the JSON
    cJSON* root = cJSON_Parse(fileData);
    if (!root) {
        //SDL_Log("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(fileData);
        return false;
    }
    // Print the root JSON for verification
    //SDL_Log("Parsed root:\n%s\n", cJSON_Print(root));

    free(fileData);

    Level level = {0};

    // 3. Extract levelName
    cJSON* levelName = cJSON_GetObjectItemCaseSensitive(root, "levelName");
    if (cJSON_IsString(levelName) && (levelName->valuestring != NULL)) {
        level.levelName = arena_strdup(&game->levelArena, levelName->valuestring);
        //SDL_Log("Level name: %s\n", level.levelName);
    } else {
        //SDL_Log("levelName is missing or not a string.\n");
        cJSON_Delete(root);
        return false;
    }

    // 4. Extract resources object
    cJSON* resources = cJSON_GetObjectItemCaseSensitive(root, "resources");
    if (cJSON_IsObject(resources)) {
        cJSON* bgTexture   = cJSON_GetObjectItemCaseSensitive(resources, "backgroundTexture");
        cJSON* spriteSheet = cJSON_GetObjectItemCaseSensitive(resources, "spriteSheet");
        cJSON* musicFile   = cJSON_GetObjectItemCaseSensitive(resources, "musicFile");

        // backgroundTexture
        if (cJSON_IsString(bgTexture) && (bgTexture->valuestring != NULL)) {
            level.resources.backgroundTexture = arena_strdup(&game->levelArena, bgTexture->valuestring);
            //SDL_Log("level.resources.backgroundTexture: %s\n", level.resources.backgroundTexture);
        } else {
            //SDL_Log("backgroundTexture is missing or not a string.\n");
            cJSON_Delete(root);
            return false;
        }

        // spriteSheet
        if (cJSON_IsString(spriteSheet) && (spriteSheet->valuestring != NULL)) {
            level.resources.spriteSheet = arena_strdup(&game->levelArena, spriteSheet->valuestring);
            //SDL_Log("level.resources.spriteSheet: %s\n", level.resources.spriteSheet);
        } else {
            //SDL_Log("spriteSheet is missing or not a string.\n");
            cJSON_Delete(root);
            return false;
        }

        // musicFile
        if (cJSON_IsString(musicFile) && (musicFile->valuestring != NULL)) {
            level.resources.musicFile = arena_strdup(&game->levelArena, musicFile->valuestring);
            //SDL_Log("level.resources.musicFile: %s\n", level.resources.musicFile);
        } else {
            //SDL_Log("musicFile is missing or not a string.\n");
            cJSON_Delete(root);
            return false;
        }
    } else {
        //SDL_Log("resources section is missing or not an object.\n");
        cJSON_Delete(root);
        return false;
    }

    {
        cJSON* grassColorsJSON = cJSON_GetObjectItemCaseSensitive(root, "grassColors");
        if (cJSON_IsObject(grassColorsJSON)) {
            // light
            cJSON* lightHex = cJSON_GetObjectItemCaseSensitive(grassColorsJSON, "light");
            if (cJSON_IsString(lightHex) && (lightHex->valuestring != NULL)) {
                level.grassOverrides.lightGrass = hexToColor(lightHex->valuestring);
                COLOR_LIGHT.grass = level.grassOverrides.lightGrass;
            } else {
                // fallback to default
                level.grassOverrides.lightGrass = COLOR_LIGHT.grass; 
            }

            // dark
            cJSON* darkHex = cJSON_GetObjectItemCaseSensitive(grassColorsJSON, "dark");
            if (cJSON_IsString(darkHex) && (darkHex->valuestring != NULL)) {
                level.grassOverrides.darkGrass = hexToColor(darkHex->valuestring);
                COLOR_DARK.grass  = level.grassOverrides.darkGrass;
            } else {
                // fallback to default
                level.grassOverrides.darkGrass = COLOR_DARK.grass; 
            }
        }
        else {
            // No grassColors object; keep the default compile-time grass
            level.grassOverrides.lightGrass = COLOR_LIGHT.grass;
            level.grassOverrides.darkGrass  = COLOR_DARK.grass;
        }
    }

    // 5. Extract background data and parse it
    cJSON* backgroundJSON = cJSON_GetObjectItemCaseSensitive(root, "background");
    if (!loadBackground(game, backgroundJSON, &level.background)) {
        //SDL_Log("Failed to load background data.\n");
        cJSON_Delete(root);
        return false;
    }

    // 6. Road building commands
    cJSON* roadJSON = cJSON_GetObjectItemCaseSensitive(root, "road");
    if (roadJSON) {
        if (!loadRoad(game, roadJSON, &level.roadData)) {
            SDL_Log("Failed to load road commands.\n");
            cJSON_Delete(root);
            return false;
        }
    } else {
        SDL_Log("No 'road' object found in JSON, skipping.\n");
    }

    // Load data into game struct field
    game->loadedRoadData.commands     = level.roadData.commands;
    game->loadedRoadData.commandCount = level.roadData.commandCount;
    // 7. Game Settings

    // 8. Apply loaded data to the game
    char path_buf[1024];

    if (!paths_resolve(path_buf, sizeof(path_buf), level.resources.backgroundTexture)) {
        SDL_Log("loadLevel: background texture not found: %s", level.resources.backgroundTexture);
        cJSON_Delete(root);
        return false;
    }
    game->background = IMG_LoadTexture(game->renderer, path_buf);
    if (!game->background) {
        SDL_Log("loadLevel: IMG_LoadTexture background: %s", IMG_GetError());
        cJSON_Delete(root);
        return false;
    }

    if (!paths_resolve(path_buf, sizeof(path_buf), level.resources.spriteSheet)) {
        SDL_Log("loadLevel: sprite sheet not found: %s", level.resources.spriteSheet);
        rollbackLevelLoadGpuResources(game);
        cJSON_Delete(root);
        return false;
    }
    game->spritesheet = IMG_LoadTexture(game->renderer, path_buf);
    if (!game->spritesheet) {
        SDL_Log("loadLevel: IMG_LoadTexture spritesheet: %s", IMG_GetError());
        rollbackLevelLoadGpuResources(game);
        cJSON_Delete(root);
        return false;
    }

    if (!paths_resolve(path_buf, sizeof(path_buf), level.resources.musicFile)) {
        SDL_Log("loadLevel: music not found: %s", level.resources.musicFile);
        rollbackLevelLoadGpuResources(game);
        cJSON_Delete(root);
        return false;
    }
    game->music = Mix_LoadMUS(path_buf);
    if (!game->music) {
        SDL_Log("loadLevel: Mix_LoadMUS: %s", Mix_GetError());
        rollbackLevelLoadGpuResources(game);
        cJSON_Delete(root);
        return false;
    }

    // Play Music
    /*
    if (Mix_PlayMusic(game->music, -1) == -1) {
        //SDL_Log("Failed to play music: %s", Mix_GetError());
        Mix_FreeMusic(game->music);
        cJSON_Delete(root);
        return false;
    }
    */
    // 8.2 (Game settings)
    // 8.3 Initialize background sliding window
    if (game->slidingWindow) {
        freeBackgroundResources(game->slidingWindow);
        free(game->slidingWindow);
        game->slidingWindow = NULL;
    }

    game->slidingWindow = (SlidingWindow*)malloc(sizeof(SlidingWindow));
    if (!game->slidingWindow) {
        //SDL_Log("Failed to allocate SlidingWindow.\n");
        rollbackLevelLoadGpuResources(game);
        cJSON_Delete(root);
        return false;
    }
    memset(game->slidingWindow, 0, sizeof(SlidingWindow));

    // Assign cycleStates
    game->slidingWindow->cycleStates      = level.background.cycleStates;
    game->slidingWindow->cycleStateCount = level.background.cycleStateCount;

    // Assign background texture to each cycle state
    for (int i = 0; i < game->slidingWindow->cycleStateCount; i++) {
        game->slidingWindow->cycleStates[i].texture = game->background;
    }

    // Initialize SlidingWindow fields
    game->slidingWindow->currentCycleStateIndex = 0;
    game->slidingWindow->targetCycleStateIndex  = (game->slidingWindow->cycleStateCount > 1) ? 1 : -1;
    game->slidingWindow->transitionProgress     = 0.0f;
    game->slidingWindow->transitioning = false;
    game->slidingWindow->transitionElapsed = false;

    // If we have at least one cycleState, initialize the blended colors
    if (game->slidingWindow->cycleStateCount > 0) {
        game->slidingWindow->interpolatedSkyColor  = game->slidingWindow->cycleStates[0].skyColor;
        game->slidingWindow->interpolatedRoadColor = game->slidingWindow->cycleStates[0].roadColor;
    }

    // Precompute transition speeds
    double maxCurvature = MAX_CURVATURE; 
    precomputeTransitionSpeeds(game->slidingWindow, game->maxSpeed, maxCurvature);

    // If more than one cycle state, set up pointers for transitions
    if (game->slidingWindow->cycleStateCount > 1) {
        game->currentCycleState = &game->slidingWindow->cycleStates[0];
        game->targetCycleState  = &game->slidingWindow->cycleStates[1];
    } else {
        // Only one state available
        game->currentCycleState = &game->slidingWindow->cycleStates[0];
        game->targetCycleState  = NULL;
    }

    if (!loadSceneryFromJson(game, cJSON_GetObjectItemCaseSensitive(root, "scenery"))) {
        SDL_Log("loadLevel: scenery allocation failed");
        rollbackLevelLoadGpuResources(game);
        cJSON_Delete(root);
        return false;
    }

    cJSON_Delete(root);
    //SDL_Log("Level '%s' loaded successfully.\n", level.levelName);
    return true;
}