// leaderboard.c
#include "leaderboard.h"
#include "game.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <math.h>
#include <string.h>
#include <stdalign.h>


// Comparison for sorting leaderboard entries by race time
static int compareLeaderboardEntries(const void* a, const void* b) {
    const LeaderboardEntry* A = (const LeaderboardEntry*)a;
    const LeaderboardEntry* B = (const LeaderboardEntry*)b;

    if (A->totalRaceTime < B->totalRaceTime) return -1;
    if (A->totalRaceTime > B->totalRaceTime) return 1;
    return 0;
}

// Validate AI state
static bool validateAIState(struct Game* game) {
    if (!game->aiController.drivers && game->aiController.driverCount > 0) {
        SDL_Log("Error: AI driver array NULL but driverCount=%d. Resetting.",
                game->aiController.driverCount);
        game->aiController.driverCount = 0;
    }

    if (game->aiController.driverCount < 0) {
        SDL_Log("Error: Negative AI driver count %d. Resetting.",
                game->aiController.driverCount);
        game->aiController.driverCount = 0;
    }

    return true;
}

// Allocate leaderboard entries using the level arena
static bool allocateLeaderboardEntries(struct Game* game) {
    int count = 1 + game->aiController.driverCount;

    if (count <= 0) {
        SDL_Log("Invalid racer count: %d", count);
        return false;
    }

    game->leaderboard.entries =
        arena_alloc(&game->levelArena, count * sizeof(LeaderboardEntry),
                    alignof(LeaderboardEntry));

    if (!game->leaderboard.entries) {
        SDL_Log("Failed to allocate leaderboard entries (%d)", count);
        return false;
    }

    memset(game->leaderboard.entries, 0, count * sizeof(LeaderboardEntry));
    game->leaderboard.entryCount = count;
    return true;
}

// Determine if the player finished first
static bool determineIfPlayerFinishedFirst(struct Game* game) {
    double pTime = game->totalRaceTime;

    for (int i = 0; i < game->aiController.driverCount; i++) {
        AIDriver* ai = &game->aiController.drivers[i];
        if (!ai) continue;
        if (!ai->finished) continue;
        if (isnan(ai->totalRaceTime)) continue;
        if (ai->totalRaceTime < pTime)
            return false;
    }

    return true;
}

// Estimate remaining times for unfinished AI drivers
static void estimateAIRemainingTimes(struct Game* game) {
    double trackLength = game->road.trackLength;

    if (trackLength <= 0 || isnan(trackLength)) {
        SDL_Log("Invalid track length; assigning fallback AI times.");
        for (int i = 0; i < game->aiController.driverCount; i++)
            game->aiController.drivers[i].totalRaceTime = 99999.0;
        return;
    }

    for (int i = 0; i < game->aiController.driverCount; i++) {
        AIDriver* ai = &game->aiController.drivers[i];

        if (ai->finished || ai->z <= 0.0 || isnan(ai->speed)) {
            if (isnan(ai->totalRaceTime))
                ai->totalRaceTime = 99999.0;
            continue;
        }

        double remaining = trackLength - ai->z;
        if (remaining < 0) remaining += trackLength;

        double effSpeed = (ai->speed > 0.1 ? ai->speed : 0.1);
        double est = remaining / effSpeed;

        float f = 0.5f + ((float)rand() / RAND_MAX) * 0.3f;
        est *= f;

        if (est < 0 || isnan(est) || isinf(est))
            ai->totalRaceTime = 99999.0;
        else
            ai->totalRaceTime += est;
    }
}

// Fill the leaderboard entry array: player then AI
static void populateLeaderboardEntries(struct Game* game) {
    LeaderboardEntry* e = game->leaderboard.entries;

    // Player is index 0
    e[0].isPlayer      = true;
    e[0].lapsCompleted = game->playerLapsCompleted;
    e[0].totalRaceTime = isnan(game->totalRaceTime) ? 99999.0 : game->totalRaceTime;
    e[0].totalDistance =
        game->playerLapsCompleted * game->road.trackLength + game->position;
    strncpy(e[0].name, "Player", 31);

    // AI entries
    for (int i = 0; i < game->aiController.driverCount; i++) {
        LeaderboardEntry* out = &e[i + 1];
        AIDriver* ai = &game->aiController.drivers[i];

        out->isPlayer      = false;
        out->lapsCompleted = ai->lapsCompleted;
        out->totalDistance =
            ai->lapsCompleted * game->road.trackLength + ai->z;
        out->totalRaceTime = ai->totalRaceTime;

        snprintf(out->name, 32, "AI %d", i + 1);
    }
}

// Sanitize race times to prevent NaN/Inf
static void sanitizeLeaderboardEntries(struct Game* game) {
    for (int i = 0; i < game->leaderboard.entryCount; i++) {
        LeaderboardEntry* e = &game->leaderboard.entries[i];

        if (e->totalRaceTime < 0 ||
            isnan(e->totalRaceTime) ||
            isinf(e->totalRaceTime) ||
            e->totalRaceTime > 36000.0)
        {
            e->totalRaceTime = 99999.0;
        }
    }
}

// Sort leaderboard entries
static void sortLeaderboardEntries(struct Game* game) {
    if (game->leaderboard.entryCount > 0)
        qsort(game->leaderboard.entries,
              game->leaderboard.entryCount,
              sizeof(LeaderboardEntry),
              compareLeaderboardEntries);
}

// Build SDL Texture containing rendered leaderboard
static void buildLeaderboardTexture(struct Game* game) {
    Leaderboard* lb = &game->leaderboard;

    if (!lb->entries || lb->entryCount <= 0 || !game->hudFont)
        return;

    const int padding = 20;
    const int entryHeight = 30;
    const int entrySpacing = 5;
    const int texWidth = 400;

    int count = lb->entryCount;
    int texHeight =
        padding * 2 + count * (entryHeight + entrySpacing) - entrySpacing;

    SDL_Surface* surf =
        SDL_CreateRGBSurfaceWithFormat(0, texWidth, texHeight, 32,
                                       SDL_PIXELFORMAT_RGBA8888);

    if (!surf) {
        SDL_Log("Failed to create leaderboard surface");
        return;
    }

    SDL_FillRect(surf, NULL, SDL_MapRGBA(surf->format, 0, 0, 0, 150));

    SDL_Color white = {255, 255, 255, 255};
    int y = padding;

    for (int i = 0; i < count; i++) {
        LeaderboardEntry* e = &lb->entries[i];

        char pos[16], timeStr[32];
        snprintf(pos, sizeof(pos), "%d", i + 1);
        snprintf(timeStr, sizeof(timeStr), "%.2fs", e->totalRaceTime);

        SDL_Surface* sPos  = TTF_RenderText_Blended(game->hudFont, pos, white);
        SDL_Surface* sName = TTF_RenderText_Blended(game->hudFont, e->name, white);
        SDL_Surface* sTime = TTF_RenderText_Blended(game->hudFont, timeStr, white);

        if (!sPos || !sName || !sTime) {
            SDL_FreeSurface(sPos);
            SDL_FreeSurface(sName);
            SDL_FreeSurface(sTime);
            continue;
        }

        SDL_Rect rPos  = {padding,        y, sPos->w,  sPos->h};
        SDL_Rect rName = {padding + 50,   y, sName->w, sName->h};
        SDL_Rect rTime = {texWidth - padding - sTime->w, y, sTime->w, sTime->h};

        SDL_BlitSurface(sPos,  NULL, surf, &rPos);
        SDL_BlitSurface(sName, NULL, surf, &rName);
        SDL_BlitSurface(sTime, NULL, surf, &rTime);

        SDL_FreeSurface(sPos);
        SDL_FreeSurface(sName);
        SDL_FreeSurface(sTime);

        y += entryHeight + entrySpacing;
    }

    if (lb->texture)
        SDL_DestroyTexture(lb->texture);

    lb->texture = SDL_CreateTextureFromSurface(game->renderer, surf);
    lb->rect.x = (game->width  - texWidth)  / 2;
    lb->rect.y = (game->height - texHeight) / 2;
    lb->rect.w = texWidth;
    lb->rect.h = texHeight;
    lb->isActive = (lb->texture != NULL);

    SDL_FreeSurface(surf);
}

// Public API — orchestrates the entire leaderboard creation
void generateLeaderboard(struct Game* game) {
    if (!validateAIState(game))
        return;

    if (!allocateLeaderboardEntries(game))
        return;

    bool playerFirst = determineIfPlayerFinishedFirst(game);

    if (playerFirst)
        estimateAIRemainingTimes(game);

    populateLeaderboardEntries(game);
    sanitizeLeaderboardEntries(game);
    sortLeaderboardEntries(game);
    buildLeaderboardTexture(game);
}
