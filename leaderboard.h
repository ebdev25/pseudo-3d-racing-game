#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <SDL2/SDL.h>
#include <stdbool.h>

struct Game;

// Leaderboard Entry structure
typedef struct {
    char name[32];
    int lapsCompleted;
    double totalDistance; // laps * trackLength + current position
    double totalRaceTime;
    bool isPlayer;
} LeaderboardEntry;

// Leaderboard structure
typedef struct {
    LeaderboardEntry* entries;
    int entryCount;
    SDL_Texture* texture;
    SDL_Rect rect;
    bool isActive;
} Leaderboard;

// Create leaderboard after race end
// Produce sorted entries, build SDL texture
void generateLeaderboard(struct Game* game);

#endif // LEADERBOARD_H