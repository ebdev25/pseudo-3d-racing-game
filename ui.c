#include "ui.h"
#include "game.h"
#include <stdio.h> // For NULL

// Helper to get options and count for the current state
static void getMenuOptions(UIState state, const char*** options, int* count) {
    static const char* startOptions[] = { "START GAME", "EXIT" };
    static const char* pauseOptions[] = { "RESUME", "RESTART", "EXIT TO MENU" };
    static const char* replayOptions[] = { "REPLAY", "BACK TO MENU" };

    switch (state) {
        case UI_STATE_START_MENU:
            *options = startOptions;
            *count = sizeof(startOptions) / sizeof(startOptions[0]);
            break;
        case UI_STATE_PAUSE_MENU:
            *options = pauseOptions;
            *count = sizeof(pauseOptions) / sizeof(pauseOptions[0]);
            break;
        case UI_STATE_REPLAY_MENU:
            *options = replayOptions;
            *count = sizeof(replayOptions) / sizeof(replayOptions[0]);
            break;
        default:
            *options = NULL;
            *count = 0;
            break;
    }
}

void initUI(UI* ui, TTF_Font* font) {
    ui->currentState = UI_STATE_NONE; // Start with no UI visible
    ui->font = font;
    ui->textColor = (SDL_Color){255, 255, 255, 255}; // White
    ui->selectedColor = (SDL_Color){255, 0, 0, 255};   // Red for selection
    ui->selectedOption = 0;
    ui->optionCount = 0;
}

// Renders menu options, center block horizontally
void renderMenuOptions(SDL_Renderer* renderer, TTF_Font* font, SDL_Color textColor, SDL_Color selectedColor,
                       int selected, const char** options, int optionCount,
                       int screenWidth, int screenHeight) {
    if (!options || optionCount <= 0) return;

    int totalHeight = 0;
    int maxWidth = 0;
    const int spacing = 10; // Vertical space between options

    // Calculate dimensions for centering
    for (int i = 0; i < optionCount; i++) {
        int w, h;
        if (TTF_SizeText(font, options[i], &w, &h) == 0) {
            totalHeight += h + spacing;
            if (w > maxWidth) {
                maxWidth = w;
            }
        }
    }
    totalHeight -= spacing; // Remove last spacing

    // Calculate starting position to center block
    int startX = (screenWidth - maxWidth) / 2;
    int startY = (screenHeight - totalHeight) / 2;
    int currentY = startY;

    // Render options
    for (int i = 0; i < optionCount; i++) {
        SDL_Color color = (i == selected) ? selectedColor : textColor;
        SDL_Surface* surface = TTF_RenderText_Blended(font, options[i], color);
        if (!surface) continue; // Handle error

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            continue; // Handle error
        }

        // Position centered horizontally within block
        SDL_Rect dst = { startX + (maxWidth - surface->w) / 2, currentY, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        currentY += surface->h + spacing;

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void renderUI(SDL_Renderer* renderer, UI* ui, int screenWidth, int screenHeight) {
    if (ui->currentState == UI_STATE_NONE || !ui->font) {
        return; // Nothing to render or no font
    }

    const char** options = NULL;

    getMenuOptions(ui->currentState, &options, &ui->optionCount); // Update count

    // Render semi-transparent background for menus
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150); // Dark semi-transparent
    SDL_Rect bgRect = { screenWidth / 4, screenHeight / 4, screenWidth / 2, screenHeight / 2 };
    SDL_RenderFillRect(renderer, &bgRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); // Reset blend mode

    // Render options
    renderMenuOptions(renderer, ui->font, ui->textColor, ui->selectedColor,
                      ui->selectedOption, options, ui->optionCount,
                      screenWidth, screenHeight);
}

UIAction handleUIInput(Game* game, UI* ui, SDL_Event* event) {
    if (ui->currentState == UI_STATE_NONE || ui->optionCount <= 0) {
        return UI_ACTION_NONE; // No UI active or no options
    }

    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_UP:
                ui->selectedOption = (ui->selectedOption - 1 + ui->optionCount) % ui->optionCount;
                // Play UI navigation sound
                if (game->uiNavigationSound) { // Check if the sound loaded successfully
                    Mix_PlayChannel(-1, game->uiNavigationSound, 0); // Play the sound on any available channel
                }
                break;
            case SDLK_DOWN:
                ui->selectedOption = (ui->selectedOption + 1) % ui->optionCount;
                // Play UI navigation sound
                if (game->uiNavigationSound) {
                    Mix_PlayChannel(-1, game->uiNavigationSound, 0);
                }
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                // Determine the action based on current state and selected option
                switch (ui->currentState) {
                    case UI_STATE_START_MENU:
                        if (ui->selectedOption == 0) return UI_ACTION_START_GAME;
                        if (ui->selectedOption == 1) return UI_ACTION_EXIT_GAME;
                        break;
                    case UI_STATE_PAUSE_MENU:
                        if (ui->selectedOption == 0) return UI_ACTION_RESUME_GAME;
                        if (ui->selectedOption == 1) return UI_ACTION_RESTART_GAME;
                        if (ui->selectedOption == 2) return UI_ACTION_EXIT_TO_MENU;
                        break;
                    case UI_STATE_REPLAY_MENU:
                        if (ui->selectedOption == 0) return UI_ACTION_REPLAY_GAME;
                        if (ui->selectedOption == 1) return UI_ACTION_BACK_TO_MENU;
                        break;
                    default:
                        break;
                }
                break;
            case SDLK_ESCAPE:
                 // Allow ESC to back out of menus
                 if(ui->currentState == UI_STATE_PAUSE_MENU) {
                    return UI_ACTION_RESUME_GAME; // Treat ESC as resume in pause menu
                 }
                 break;
        }
    }

    return UI_ACTION_NONE; // Default: no action taken
}

void setUIState(UI* ui, UIState state) {
    ui->currentState = state;
    ui->selectedOption = 0; // Reset selection when changing state

    // Update option count based on the new state
    const char** options; // Dummy pointer
    getMenuOptions(state, &options, &ui->optionCount);
}