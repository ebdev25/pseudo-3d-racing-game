#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

struct Game;

typedef enum {
    UI_STATE_NONE,
    UI_STATE_START_MENU,
    UI_STATE_PAUSE_MENU,
    UI_STATE_REPLAY_MENU
} UIState;

// Actions returned by handleUIInput when Enter is pressed
typedef enum {
    UI_ACTION_NONE,         // No action selected / No input
    // Start Menu Actions
    UI_ACTION_START_GAME,
    UI_ACTION_EXIT_GAME,    // From start menu
    // Pause Menu Actions
    UI_ACTION_RESUME_GAME,
    UI_ACTION_RESTART_GAME, // From pause menu
    UI_ACTION_EXIT_TO_MENU, // From pause menu
    // Replay Menu Actions
    UI_ACTION_REPLAY_GAME,
    UI_ACTION_BACK_TO_MENU  // From replay menu
} UIAction;


typedef struct {
    UIState currentState;
    TTF_Font* font;
    SDL_Color textColor;
    SDL_Color selectedColor; // Color for the selected option
    int selectedOption;
    int optionCount;        // Track the number of options in the current menu
} UI;

void initUI(UI* ui, TTF_Font* font);
void renderUI(SDL_Renderer* renderer, UI* ui, int screenWidth, int screenHeight);
UIAction handleUIInput(struct Game* game, UI* ui, SDL_Event* event);
void setUIState(UI* ui, UIState state);

#endif // UI_H