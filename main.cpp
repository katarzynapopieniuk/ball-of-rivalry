//
// Created by Kasia on 18.03.2023.
//

#include "main.h"
#include "PlayerCharacter.h"

#include <iostream>
#include <memory>
#include <array>
#include <cmath>

#define SDL_MAIN_HANDLED
#define PLAYGROUND_WIDTH 650
#define PLAYGROUND_HEIGHT 480
#define SCOREBOARD_WIDTH 650
#define SCOREBOARD_HEIGHT 390
#define WINDOW_WIDTH 650
#define WINDOW_HEIGHT 870
#define MAX_DUST_AMOUNT 20
#define WINNER_PLAYER_NUMBER_HEIGHT 140
#define WINNER_PLAYER_NUMBER_WIDTH 410

#include <SDL.h>
#include "operator_definitions.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

const int TICK_TIME = 33;
const int DUST_SPAWN_TICKS = 100;
const int PLAYER_SCORE_HEIGHT = 300;
const int TIME_HEIGHT = 120;
const int LEFT_SCORE_START_WIDTH = 260;
const int RIGHT_SCORE_START_WIDTH = 350;
const int SCORE_JUMP = 50;
const int TOTAL_GAME_TIME_IN_SECONDS = 90;

std::pair<PlayerCharacter, PlayerCharacter> updatePlayersIfCollision(PlayerCharacter firstPlayer, PlayerCharacter secondPlayer,
                                                                     PlayerCharacter firstPlayerUpdated, PlayerCharacter secondPlayerUpdated, int playerSize);

PlayerCharacter updatePlayerIfWallCollision(PlayerCharacter playerCharacter, int i);

void spawnDust(vec2d dustPositions[20], int dustIndex, int dustSize) ;

bool
playerScored(PlayerCharacter playerCharacter, vec2d dustPositions[20], int dustAmount, int playerSize, int dustSize);

std::shared_ptr<SDL_Texture> load_texture(SDL_Renderer *renderer, const std::string& fname) {
    SDL_Surface *bmpSurface = SDL_LoadBMP(("assets/" + fname).c_str());
    if (!bmpSurface) {
        throw std::invalid_argument("Could not load bmpSurface");
    }
    SDL_SetColorKey(bmpSurface, SDL_TRUE, 0x0ff00ff);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, bmpSurface);
    if (!texture) {
        throw std::invalid_argument("Could not create texture");
    }
    SDL_FreeSurface(bmpSurface);
    return {texture, [](SDL_Texture *tex) {
        SDL_DestroyTexture(tex);
    }};
}

std::pair<int, int> get_texture_width_and_height(const std::shared_ptr<SDL_Texture>& texture) {
    int width, height;
    SDL_QueryTexture(texture.get(), nullptr, nullptr, &width, &height);
    return {width, height};
}

SDL_Rect get_texture_rect(std::shared_ptr<SDL_Texture> texture) {
    auto [width, height] = get_texture_width_and_height(texture);
    return {0, 0, width, height};
}

using vec2d = std::array<double, 2>;

vec2d angle_to_vector(double angle) {
    return {std::cos(angle), std::sin(angle)};
}

double dot(vec2d first, vec2d second) {
    return first[0] * second[0] + first[1] * second[1];
}

vec2d
acceleration_vector_from_keyboard_and_player(const PlayerCharacter &player, SDL_Scancode scanCodeForward, SDL_Scancode scanCodeBack) {
    auto *keyboard_state = SDL_GetKeyboardState(nullptr);
    vec2d forward_vec = angle_to_vector(player.angle);
    vec2d acceleration = {0, 0};
    if (keyboard_state[scanCodeForward]) {
        acceleration = acceleration + forward_vec;
    }
    if (keyboard_state[scanCodeBack]) {
        acceleration = acceleration - forward_vec;
    }
    return acceleration*1500.0;
}

double angle_from_keyboard_and_player(const PlayerCharacter &player, SDL_Scancode scanCodeLeft, SDL_Scancode scanCodeRight) {
    auto *keyboard_state = SDL_GetKeyboardState(nullptr);
    double angle = player.angle;
    if (keyboard_state[scanCodeLeft]) angle = angle - M_PI / 10.0;
    if (keyboard_state[scanCodeRight]) angle = angle + M_PI / 10.0;
    return angle;
}

void play_the_game(SDL_Renderer *renderer) {
    auto firstPlayerTexture = load_texture(renderer, "player1.bmp");
    auto secondPlayerTexture = load_texture(renderer, "player2.bmp");
    auto background = load_texture(renderer, "wooden_floor.bmp");
    auto dust_texture = load_texture(renderer, "dust.bmp");
    auto scoreboard_texture = load_texture(renderer, "scoreboard.bmp");
    auto tieboard_texture = load_texture(renderer, "tieboard.bmp");
    auto winnerboard_texture = load_texture(renderer, "winnerboard.bmp");
    auto number0_texture = load_texture(renderer, "number0.bmp");
    auto number1_texture = load_texture(renderer, "number1.bmp");
    auto number2_texture = load_texture(renderer, "number2.bmp");
    auto number3_texture = load_texture(renderer, "number3.bmp");
    auto number4_texture = load_texture(renderer, "number4.bmp");
    auto number5_texture = load_texture(renderer, "number5.bmp");
    auto number6_texture = load_texture(renderer, "number6.bmp");
    auto number7_texture = load_texture(renderer, "number7.bmp");
    auto number8_texture = load_texture(renderer, "number8.bmp");
    auto number9_texture = load_texture(renderer, "number9.bmp");
    std::shared_ptr<SDL_Texture> numberTextures[10] = {number0_texture, number1_texture, number2_texture,
                                                        number3_texture, number4_texture, number5_texture,
                                                        number6_texture, number7_texture, number8_texture,
                                                        number9_texture};

    SDL_Rect firstPlayerRect = get_texture_rect(firstPlayerTexture);
    SDL_Rect secondPlayerRect = get_texture_rect(secondPlayerTexture);
    SDL_Rect dustRect = get_texture_rect(dust_texture);
    SDL_Rect scoreboardRect = get_texture_rect(scoreboard_texture);
    SDL_Rect tieboardRect = get_texture_rect(tieboard_texture);
    SDL_Rect winnerboardRect = get_texture_rect(winnerboard_texture);

    SDL_Rect numberRects[10];
    for(int i=0; i<10; i++) {
        numberRects[i] = get_texture_rect(numberTextures[i]);
    }

    auto playerSize = firstPlayerRect.w;
    PlayerCharacter firstPlayer = {0, {300.0, 200.0}};
    PlayerCharacter secondPlayer = {0, {400.0, 200.0}};
    int gaming = true;
    auto prev_tick = SDL_GetTicks();
    int ticksTillNextDustSpawn = DUST_SPAWN_TICKS;
    vec2d dustPositions[MAX_DUST_AMOUNT] = {};
    int dustAmount = 0;
    int dustSize = dustRect.w;
    time_t startTime, currentTime;
    time (&startTime);
    srand ((unsigned)time(NULL));
    while (gaming) {
        SDL_Event sdlEvent;

        while (SDL_PollEvent(&sdlEvent) != 0) {
            switch (sdlEvent.type) {
                case SDL_QUIT:
                    gaming = false;
                    break;
                case SDL_KEYDOWN:
                    if (sdlEvent.key.keysym.sym == SDLK_q) gaming = false;
                    break;
            }
        }

        {
            ticksTillNextDustSpawn--;
            if(ticksTillNextDustSpawn == 0) {
                ticksTillNextDustSpawn = DUST_SPAWN_TICKS;
                if(dustAmount < MAX_DUST_AMOUNT) {
                    spawnDust(dustPositions, dustAmount, dustSize);
                    dustAmount ++;
                }
            }
        }

        {
            firstPlayer.acceleration = acceleration_vector_from_keyboard_and_player(firstPlayer, SDL_SCANCODE_UP,
                                                                                    SDL_SCANCODE_DOWN);
            firstPlayer.angle = angle_from_keyboard_and_player(firstPlayer, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
            PlayerCharacter firstPlayerUpdated = firstPlayer.next_state(TICK_TIME);

            secondPlayer.acceleration = acceleration_vector_from_keyboard_and_player(secondPlayer, SDL_SCANCODE_W,
                                                                                     SDL_SCANCODE_S);
            secondPlayer.angle = angle_from_keyboard_and_player(secondPlayer, SDL_SCANCODE_A, SDL_SCANCODE_D);
            PlayerCharacter secondPlayerUpdated = secondPlayer.next_state(TICK_TIME);

            auto players = updatePlayersIfCollision(firstPlayer, secondPlayer, firstPlayerUpdated, secondPlayerUpdated, playerSize);
            firstPlayer = updatePlayerIfWallCollision(players.first, playerSize);
            secondPlayer = updatePlayerIfWallCollision(players.second, playerSize);
        }

        {
            if(playerScored(firstPlayer, dustPositions, dustAmount, playerSize, dustSize)) {
                firstPlayer.points = firstPlayer.points + 1;
                dustAmount--;
            }

            if(playerScored(secondPlayer, dustPositions, dustAmount, playerSize, dustSize)) {
                secondPlayer.points = secondPlayer.points + 1;
                dustAmount--;
            }
        }

        SDL_RenderCopy(renderer, background.get(), nullptr, nullptr);

        for(int i = 0; i < dustAmount; i++) {
            auto rect = dustRect;
            rect.x = dustPositions[i][0] - rect.w / 2;
            rect.y = dustPositions[i][1] - rect.h / 2;
            SDL_RenderCopyEx(renderer, dust_texture.get(),
                             nullptr, &rect, 0,
                             nullptr, SDL_FLIP_NONE);
        }

        {
            auto rect = firstPlayerRect;

            rect.x = firstPlayer.position[0] - rect.w / 2;
            rect.y = firstPlayer.position[1] - rect.h / 2;
            SDL_RenderCopyEx(renderer, firstPlayerTexture.get(),
                             nullptr, &rect, 180.0 * firstPlayer.angle / M_PI,
                             nullptr, SDL_FLIP_NONE);
        }

        {
            auto rect = secondPlayerRect;

            rect.x = secondPlayer.position[0] - rect.w / 2;
            rect.y = secondPlayer.position[1] - rect.h / 2;
            SDL_RenderCopyEx(renderer, secondPlayerTexture.get(),
                             nullptr, &rect, 180.0 * secondPlayer.angle / M_PI,
                             nullptr, SDL_FLIP_NONE);
        }

        {
            auto rect = scoreboardRect;

            rect.x = 0;
            rect.y = PLAYGROUND_HEIGHT + 1;
            SDL_RenderCopyEx(renderer, scoreboard_texture.get(),
                             nullptr, &rect, 0,
                             nullptr, SDL_FLIP_NONE);
        }

        {
            auto score = firstPlayer.points;
            int positionX = LEFT_SCORE_START_WIDTH;
            do {
                auto number = score % 10;

                auto rect = numberRects[number];
                rect.x = positionX;
                rect.y = PLAYER_SCORE_HEIGHT + PLAYGROUND_HEIGHT + 1;
                SDL_RenderCopyEx(renderer, numberTextures[number].get(),
                                 nullptr, &rect, 0,
                                 nullptr, SDL_FLIP_NONE);

                score /= 10;
                positionX -= SCORE_JUMP;
            } while (score > 0);
        }

        {
            auto score = secondPlayer.points;
            int howManyDigits = 0;
            int temp = score;
            do {
                temp /= 10;
                howManyDigits ++;
            } while (temp > 0);

            int positionX = RIGHT_SCORE_START_WIDTH + (howManyDigits-1) * SCORE_JUMP;
            do {
                auto number = score % 10;

                auto rect = numberRects[number];
                rect.x = positionX;
                rect.y = PLAYER_SCORE_HEIGHT + PLAYGROUND_HEIGHT + 1;
                SDL_RenderCopyEx(renderer, numberTextures[number].get(),
                                 nullptr, &rect, 0,
                                 nullptr, SDL_FLIP_NONE);

                score /= 10;
                positionX -= SCORE_JUMP;
            } while (score > 0);
        }

        {
            time(&currentTime);
            int timeElapsedInSeconds = difftime (currentTime, startTime);
            int timeToEndInSeconds = TOTAL_GAME_TIME_IN_SECONDS - timeElapsedInSeconds;
            int minutesLeft = timeToEndInSeconds / 60;
            int secondsLeft = timeToEndInSeconds % 60;

            {
                int positionX = LEFT_SCORE_START_WIDTH;
                do {
                    auto number = minutesLeft % 10;

                    auto rect = numberRects[number];
                    rect.x = positionX;
                    rect.y = TIME_HEIGHT + PLAYGROUND_HEIGHT + 1;
                    SDL_RenderCopyEx(renderer, numberTextures[number].get(),
                                     nullptr, &rect, 0,
                                     nullptr, SDL_FLIP_NONE);

                    minutesLeft /= 10;
                    positionX -= SCORE_JUMP;
                } while (minutesLeft > 0);
            }

            {
                int positionX = RIGHT_SCORE_START_WIDTH + SCORE_JUMP;
                for(int i = 0; i < 2; i++) {
                    auto number = secondsLeft % 10;

                    auto rect = numberRects[number];
                    rect.x = positionX;
                    rect.y = TIME_HEIGHT + PLAYGROUND_HEIGHT + 1;
                    SDL_RenderCopyEx(renderer, numberTextures[number].get(),
                                     nullptr, &rect, 0,
                                     nullptr, SDL_FLIP_NONE);

                    secondsLeft /= 10;
                    positionX -= SCORE_JUMP;
                }
            }
            if(timeToEndInSeconds <= 0) {
                gaming = 0;
            }

        }

        SDL_RenderPresent(renderer);

        auto current_tick = SDL_GetTicks();
        SDL_Delay(TICK_TIME - (current_tick - prev_tick));
        prev_tick += TICK_TIME;
    }

    while(true) {
        SDL_Event sdlEvent;

        while (SDL_PollEvent(&sdlEvent) != 0) {
            switch (sdlEvent.type) {
                case SDL_QUIT:
                    gaming = false;
                    break;
                case SDL_KEYDOWN:
                    if (sdlEvent.key.keysym.sym == SDLK_q) gaming = false;
                    break;
            }
        }

        if(firstPlayer.points == secondPlayer.points) {
            auto rect = tieboardRect;

            rect.x = 0;
            rect.y = PLAYGROUND_HEIGHT + 1;
            SDL_RenderCopyEx(renderer, tieboard_texture.get(),
                             nullptr, &rect, 0,
                             nullptr, SDL_FLIP_NONE);
        } else {
            auto rect = winnerboardRect;

            rect.x = 0;
            rect.y = PLAYGROUND_HEIGHT + 1;
            SDL_RenderCopyEx(renderer, winnerboard_texture.get(),
                             nullptr, &rect, 0,
                             nullptr, SDL_FLIP_NONE);
        }

        if(firstPlayer.points > secondPlayer.points) {
            auto number = 1;
            auto rect = numberRects[number];
            rect.x = WINNER_PLAYER_NUMBER_WIDTH;
            rect.y = WINNER_PLAYER_NUMBER_HEIGHT + PLAYGROUND_HEIGHT + 1;
            SDL_RenderCopyEx(renderer, numberTextures[number].get(),
                             nullptr, &rect, 0,
                             nullptr, SDL_FLIP_NONE);
        } else if(firstPlayer.points < secondPlayer.points) {
            auto number = 2;
            auto rect = numberRects[number];
            rect.x = WINNER_PLAYER_NUMBER_WIDTH;
            rect.y = WINNER_PLAYER_NUMBER_HEIGHT + PLAYGROUND_HEIGHT + 1;
            SDL_RenderCopyEx(renderer, numberTextures[number].get(),
                             nullptr, &rect, 0,
                             nullptr, SDL_FLIP_NONE);
        }

        SDL_RenderPresent(renderer);
    }
}

bool playerScored(PlayerCharacter playerCharacter, vec2d dustPositions[20], int dustAmount, int playerSize, int dustSize) {
    for(int i = 0; i < dustAmount; i++) {
        if(playerCharacter.getDistance(dustPositions[i]) < playerSize - dustSize) {
            for(int j = i; j < dustAmount-1; j++) {
                dustPositions[j] = dustPositions[j+1];
            }
            return true;
        }
    }

    return false;
}

void spawnDust(vec2d dustPositions[20], int dustIndex, int dustSize) {
    dustPositions[dustIndex] = {static_cast<double>(rand() % (PLAYGROUND_WIDTH - dustSize) + dustSize / 2), static_cast<double>(rand() % (PLAYGROUND_WIDTH - dustSize) + dustSize / 2)};
}

std::pair<PlayerCharacter, PlayerCharacter> updatePlayersIfCollision(PlayerCharacter firstPlayer, PlayerCharacter secondPlayer, PlayerCharacter firstPlayerUpdated,
                              PlayerCharacter secondPlayerUpdated, int playerSize) {
    vec2d dir = secondPlayer.position - firstPlayer.position;
    double distance = firstPlayerUpdated.getDistance(secondPlayerUpdated);
    if(distance >= playerSize) {
        return {firstPlayerUpdated, secondPlayerUpdated};
    }

    dir = dir / distance;

    double v1 = dot(firstPlayer.velocity, dir);
    double v2 = dot(secondPlayer.velocity, dir);

    double restitution = 0.97;
    double newV1 = (v1 + v2 - (v1 - v2) * restitution) / 2;
    double newV2 = (v1 + v2 - (v2 - v1) * restitution) / 2;

    firstPlayerUpdated.acceleration = firstPlayer.acceleration;
    firstPlayerUpdated.velocity = firstPlayer.velocity + dir * (newV1 - v1);
    firstPlayerUpdated.position = firstPlayer.position;
    secondPlayerUpdated.acceleration = secondPlayer.acceleration;
    secondPlayerUpdated.velocity = secondPlayer.velocity + dir * (newV2 - v2);
    secondPlayerUpdated.position = secondPlayer.position;

    return {firstPlayerUpdated, secondPlayerUpdated};
}

PlayerCharacter updatePlayerIfWallCollision(PlayerCharacter playerCharacter, int playerSize) {
    if (playerCharacter.position[0] < playerSize / 2) {
        playerCharacter.position[0] = playerSize / 2;
        playerCharacter.velocity[0] = -playerCharacter.velocity[0];
    }
    if (playerCharacter.position[0] > PLAYGROUND_WIDTH - playerSize / 2) {
        playerCharacter.position[0] = PLAYGROUND_WIDTH - playerSize / 2;
        playerCharacter.velocity[0] = -playerCharacter.velocity[0];
    }
    if (playerCharacter.position[1] < playerSize / 2) {
        playerCharacter.position[1] = playerSize / 2;
        playerCharacter.velocity[1] = -playerCharacter.velocity[1];
    }

    if (playerCharacter.position[1] > PLAYGROUND_HEIGHT - playerSize / 2) {
        playerCharacter.position[1] = PLAYGROUND_HEIGHT - playerSize / 2;
        playerCharacter.velocity[1] = -playerCharacter.velocity[1];
    }

    return playerCharacter;
}

int main() {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT,
                                SDL_WINDOW_SHOWN,
                                &window, &renderer);

    play_the_game(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}