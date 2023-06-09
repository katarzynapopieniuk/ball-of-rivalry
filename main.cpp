//
// Created by Kasia on 18.03.2023.
//

#include "main.h"
#include "PlayerCharacter.h"

#include <iostream>
#include <memory>
#include <array>
#include <cmath>
#include <sdl_mixer.h>

#define SDL_MAIN_HANDLED
#define PLAYGROUND_WIDTH 650
#define PLAYGROUND_HEIGHT 480
#define SCOREBOARD_WIDTH 650
#define SCOREBOARD_HEIGHT 390
#define WINDOW_WIDTH 650
#define WINDOW_HEIGHT 870
#define MAX_DUST_AMOUNT 20
#define MAX_TURBO_AMOUNT 5
#define WINNER_PLAYER_NUMBER_HEIGHT 140
#define WINNER_PLAYER_NUMBER_WIDTH 410
#define MAX_BUBBLES_ON_MAP 30
#define BUBBLE_LIFE_TICKS 10

#include <SDL.h>
#include "operator_definitions.h"
#include "Bubble.h"
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
const int HOLES_AMOUNT = 4;
const int TIME_TO_RESPAWN_AFTER_DEATH_IN_SECONDS = 3;
const int FAKE_POSITION_IF_FELL = -99999;
const int TURBO_SPAWN_TICKS = 500;
const int TURBO_WORKING_TICKS = 300;
const int BUBBLE_SPAWN_TICKS = 1;
const vec2d HOLES_POSITIONS[] = {{80,  380},
                                 {500, 380},
                                 {100, 100},
                                 {480, 100}};

std::pair<PlayerCharacter, PlayerCharacter>
updatePlayersIfCollision(PlayerCharacter firstPlayer, PlayerCharacter secondPlayer,
                         PlayerCharacter firstPlayerUpdated, PlayerCharacter secondPlayerUpdated, int playerSize);

PlayerCharacter updatePlayerIfWallCollision(PlayerCharacter playerCharacter, int i);

void spawnDust(vec2d dustPositions[20], int dustIndex, int dustSize, int holeSize);

void spawnTurbo(vec2d turboPositions[MAX_TURBO_AMOUNT], int turboIndex, int turboWidth, int turboHeight);

bool
playerScored(PlayerCharacter playerCharacter, vec2d dustPositions[20], int dustAmount, int playerSize, int dustSize);

bool playerFellIntoHole(PlayerCharacter playerCharacter, int playerSize, int holeSize);

bool
playerGotTurbo(PlayerCharacter playerCharacter, vec2d turboPositions[MAX_TURBO_AMOUNT], int turboAmount, int playerSize,
               int turboWidth, int turboHeight);

void spawnBubble(Bubble bubbles[10], int bubbleIndex, PlayerCharacter player, int playerSize, int bubbleSize);

int clearDeadBubblesAndGetBubbleAmount(Bubble bubbles[MAX_BUBBLES_ON_MAP], int bubbleAmount);

bool isNewDustOnHole(vec2d dustPosition, int holeSize);

double getDistance(vec2d basePosition, vec2d comparedPosition);

std::shared_ptr<SDL_Texture> load_texture(SDL_Renderer *renderer, const std::string &fname) {
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

std::pair<int, int> get_texture_width_and_height(const std::shared_ptr<SDL_Texture> &texture) {
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
acceleration_vector_from_keyboard_and_player(const PlayerCharacter &player, SDL_Scancode scanCodeForward,
                                             SDL_Scancode scanCodeBack) {
    auto *keyboard_state = SDL_GetKeyboardState(nullptr);
    vec2d forward_vec = angle_to_vector(player.angle);
    vec2d acceleration = {0, 0};
    if (keyboard_state[scanCodeForward]) {
        acceleration = acceleration + forward_vec;
    }
    if (keyboard_state[scanCodeBack]) {
        acceleration = acceleration - forward_vec;
    }
    if (player.ticksTillTurboStopsWorking > 0) {
        return acceleration * 4000.0;
    }
    return acceleration * 1500.0;
}

double
angle_from_keyboard_and_player(const PlayerCharacter &player, SDL_Scancode scanCodeLeft, SDL_Scancode scanCodeRight) {
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
    auto hole_texture = load_texture(renderer, "hole.bmp");
    auto dust_texture = load_texture(renderer, "dust.bmp");
    auto bubble_texture = load_texture(renderer, "bubble.bmp");
    auto scoreboard_texture = load_texture(renderer, "scoreboard.bmp");
    auto tieboard_texture = load_texture(renderer, "tieboard.bmp");
    auto winnerboard_texture = load_texture(renderer, "winnerboard.bmp");
    auto controlboard_texture = load_texture(renderer, "controlboard.bmp");
    auto menu_texture = load_texture(renderer, "menu.bmp");
    auto option_frame_texture = load_texture(renderer, "option_frame.bmp");
    auto turbo_texture = load_texture(renderer, "turbo.bmp");
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
    SDL_Rect holeRect = get_texture_rect(hole_texture);
    SDL_Rect turboRect = get_texture_rect(turbo_texture);
    SDL_Rect bubbleRect = get_texture_rect(bubble_texture);
    SDL_Rect menuRect = get_texture_rect(menu_texture);
    SDL_Rect optionFrameRect = get_texture_rect(option_frame_texture);
    SDL_Rect controlboardRect = get_texture_rect(controlboard_texture);

    SDL_Rect numberRects[10];
    for (int i = 0; i < 10; i++) {
        numberRects[i] = get_texture_rect(numberTextures[i]);
    }

    Mix_Chunk *dustSuckSound = Mix_LoadWAV("./assets/mixkit-air-zoom-vacuum-2608.wav");
    Mix_Chunk *turboSound = Mix_LoadWAV("./assets/mixkit-cinematic-laser-gun-thunder-1287.wav");

    auto playerSize = firstPlayerRect.w;
    PlayerCharacter firstPlayer = {0, {400.0, 200.0}};
    PlayerCharacter secondPlayer = {0, {250.0, 200.0}};
    int gaming = true;
    auto prev_tick = SDL_GetTicks();
    int ticksTillNextDustSpawn = DUST_SPAWN_TICKS;
    int ticksTillNextTurboSpawn = TURBO_SPAWN_TICKS;
    int ticksTillNextFirstPlayerBubble = 0;
    int ticksTillNextSecondPlayerBubble = 0;
    vec2d dustPositions[MAX_DUST_AMOUNT] = {};
    vec2d turboPositions[MAX_TURBO_AMOUNT] = {};
    Bubble bubbles[MAX_BUBBLES_ON_MAP] = {};
    int dustAmount = 0;
    int turboAmount = 0;
    int bubbleAmount = 0;
    int dustSize = dustRect.w;
    int holeSize = holeRect.w;
    int bubbleSize = bubbleRect.w;
    time_t startTime, currentTime, firstPlayerDeathTime, secondPlayerDeathTime;
    time(&startTime);
    srand((unsigned) time(NULL));
    bool firstPlayerDied = false;
    bool secondPlayerDied = false;
    bool playerChoseMenuStartOption = true;
    bool isGameGoingOn = false;
    bool shouldShowPreviousResult = false;

    while (gaming) {
        while (isGameGoingOn) {
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
                if (ticksTillNextDustSpawn == 0) {
                    ticksTillNextDustSpawn = DUST_SPAWN_TICKS;
                    if (dustAmount < MAX_DUST_AMOUNT) {
                        spawnDust(dustPositions, dustAmount, dustSize, holeSize);
                        dustAmount++;
                    }
                }
            }

            {
                ticksTillNextTurboSpawn--;
                if (ticksTillNextTurboSpawn == 0) {
                    ticksTillNextTurboSpawn = TURBO_SPAWN_TICKS;
                    if (turboAmount < MAX_TURBO_AMOUNT) {
                        spawnTurbo(turboPositions, turboAmount, turboRect.w, turboRect.h);
                        turboAmount++;
                    }
                }
            }

            {
                if (!firstPlayerDied) {
                    firstPlayer.acceleration = acceleration_vector_from_keyboard_and_player(firstPlayer,
                                                                                            SDL_SCANCODE_UP,
                                                                                            SDL_SCANCODE_DOWN);
                    firstPlayer.angle = angle_from_keyboard_and_player(firstPlayer, SDL_SCANCODE_LEFT,
                                                                       SDL_SCANCODE_RIGHT);
                }

                PlayerCharacter firstPlayerUpdated = firstPlayer.next_state(TICK_TIME);

                if (!secondPlayerDied) {
                    secondPlayer.acceleration = acceleration_vector_from_keyboard_and_player(secondPlayer,
                                                                                             SDL_SCANCODE_W,
                                                                                             SDL_SCANCODE_S);
                    secondPlayer.angle = angle_from_keyboard_and_player(secondPlayer, SDL_SCANCODE_A, SDL_SCANCODE_D);
                }
                PlayerCharacter secondPlayerUpdated = secondPlayer.next_state(TICK_TIME);

                if (!firstPlayerDied && !secondPlayerDied) {
                    auto players = updatePlayersIfCollision(firstPlayer, secondPlayer, firstPlayerUpdated,
                                                            secondPlayerUpdated, playerSize);
                    firstPlayer = updatePlayerIfWallCollision(players.first, playerSize);
                    secondPlayer = updatePlayerIfWallCollision(players.second, playerSize);
                } else if (!firstPlayerDied) {
                    firstPlayer = updatePlayerIfWallCollision(firstPlayerUpdated, playerSize);
                } else if (!secondPlayerDied) {
                    secondPlayer = updatePlayerIfWallCollision(secondPlayerUpdated, playerSize);
                }
            }

            {
                if (playerScored(firstPlayer, dustPositions, dustAmount, playerSize, dustSize)) {
                    firstPlayer.points = firstPlayer.points + 1;
                    dustAmount--;
                    Mix_PlayChannel(-1, dustSuckSound, 0);
                }

                if (playerScored(secondPlayer, dustPositions, dustAmount, playerSize, dustSize)) {
                    secondPlayer.points = secondPlayer.points + 1;
                    dustAmount--;
                    Mix_PlayChannel(-1, dustSuckSound, 0);
                }
            }

            {
                if (playerGotTurbo(firstPlayer, turboPositions, turboAmount, playerSize, turboRect.w, turboRect.h)) {
                    firstPlayer.ticksTillTurboStopsWorking = TURBO_WORKING_TICKS;
                    turboAmount--;
                    ticksTillNextFirstPlayerBubble = 0;
                    Mix_PlayChannel(-1, turboSound, 0);
                }

                if (playerGotTurbo(secondPlayer, turboPositions, turboAmount, playerSize, turboRect.w, turboRect.h)) {
                    secondPlayer.ticksTillTurboStopsWorking = TURBO_WORKING_TICKS;
                    turboAmount--;
                    ticksTillNextSecondPlayerBubble = 0;
                    Mix_PlayChannel(-1, turboSound, 0);
                }
            }

            {
                if (playerFellIntoHole(firstPlayer, playerSize, holeSize)) {
                    firstPlayer.points = firstPlayer.points - 3;
                    if (firstPlayer.points < 0) {
                        firstPlayer.points = 0;
                    }
                    firstPlayerDied = true;
                    firstPlayer.position = {FAKE_POSITION_IF_FELL, FAKE_POSITION_IF_FELL};
                    time(&firstPlayerDeathTime);
                }

                if (playerFellIntoHole(secondPlayer, playerSize, holeSize)) {
                    secondPlayer.points = secondPlayer.points - 3;
                    if (secondPlayer.points < 0) {
                        secondPlayer.points = 0;
                    }
                    secondPlayerDied = true;
                    secondPlayer.position = {FAKE_POSITION_IF_FELL, FAKE_POSITION_IF_FELL};
                    time(&secondPlayerDeathTime);
                }
            }

            if (firstPlayer.ticksTillTurboStopsWorking > 0) {
                if (ticksTillNextFirstPlayerBubble <= 0 && bubbleAmount < MAX_BUBBLES_ON_MAP) {
                    spawnBubble(bubbles, bubbleAmount, firstPlayer, playerSize, bubbleSize);
                    ticksTillNextFirstPlayerBubble = BUBBLE_SPAWN_TICKS;
                    bubbleAmount++;
                } else {
                    ticksTillNextFirstPlayerBubble--;
                }
            }

            if (secondPlayer.ticksTillTurboStopsWorking > 0) {
                if (ticksTillNextSecondPlayerBubble <= 0 && bubbleAmount < MAX_BUBBLES_ON_MAP) {
                    spawnBubble(bubbles, bubbleAmount, secondPlayer, playerSize, bubbleSize);
                    ticksTillNextSecondPlayerBubble = BUBBLE_SPAWN_TICKS;
                    bubbleAmount++;
                } else {
                    ticksTillNextSecondPlayerBubble--;
                }
            }
            bubbleAmount = clearDeadBubblesAndGetBubbleAmount(bubbles, bubbleAmount);

            {
                if (firstPlayerDied) {
                    time(&currentTime);
                    int timeElapsedInSeconds = difftime(currentTime, firstPlayerDeathTime);
                    if (timeElapsedInSeconds >= TIME_TO_RESPAWN_AFTER_DEATH_IN_SECONDS) {
                        firstPlayerDied = false;
                        firstPlayer.position = {400.0, 200.0};
                        if (firstPlayer.getDistance(secondPlayer) < playerSize) {
                            firstPlayer.position = {250.0, 200.0};
                        }
                    }
                }

                if (secondPlayerDied) {
                    time(&currentTime);
                    int timeElapsedInSeconds = difftime(currentTime, secondPlayerDeathTime);
                    if (timeElapsedInSeconds >= TIME_TO_RESPAWN_AFTER_DEATH_IN_SECONDS) {
                        secondPlayerDied = false;
                        secondPlayer.position = {250.0, 200.0};
                        if (secondPlayer.getDistance(firstPlayer) < playerSize) {
                            secondPlayer.position = {400.0, 200.0};
                        }
                    }
                }

                for (int i = 0; i < bubbleAmount; i++) {
                    bubbles[i] = bubbles[i].next_state(TICK_TIME);
                }
            }

            SDL_RenderCopy(renderer, background.get(), nullptr, nullptr);

            for (int i = 0; i < HOLES_AMOUNT; i++) {
                auto rect = holeRect;
                rect.x = HOLES_POSITIONS[i][0] - rect.w / 2;
                rect.y = HOLES_POSITIONS[i][1] - rect.h / 2;
                SDL_RenderCopyEx(renderer, hole_texture.get(),
                                 nullptr, &rect, 0,
                                 nullptr, SDL_FLIP_NONE);
            }

            for (int i = 0; i < dustAmount; i++) {
                auto rect = dustRect;
                rect.x = dustPositions[i][0] - rect.w / 2;
                rect.y = dustPositions[i][1] - rect.h / 2;
                SDL_RenderCopyEx(renderer, dust_texture.get(),
                                 nullptr, &rect, 0,
                                 nullptr, SDL_FLIP_NONE);
            }

            for (int i = 0; i < turboAmount; i++) {
                auto rect = turboRect;
                rect.x = turboPositions[i][0] - rect.w / 2;
                rect.y = turboPositions[i][1] - rect.h / 2;
                SDL_RenderCopyEx(renderer, turbo_texture.get(),
                                 nullptr, &rect, 0,
                                 nullptr, SDL_FLIP_NONE);
            }

            if (!firstPlayerDied) {
                auto rect = firstPlayerRect;

                rect.x = firstPlayer.position[0] - rect.w / 2;
                rect.y = firstPlayer.position[1] - rect.h / 2;
                SDL_RenderCopyEx(renderer, firstPlayerTexture.get(),
                                 nullptr, &rect, 180.0 * firstPlayer.angle / M_PI,
                                 nullptr, SDL_FLIP_NONE);
            }

            if (!secondPlayerDied) {
                auto rect = secondPlayerRect;

                rect.x = secondPlayer.position[0] - rect.w / 2;
                rect.y = secondPlayer.position[1] - rect.h / 2;
                SDL_RenderCopyEx(renderer, secondPlayerTexture.get(),
                                 nullptr, &rect, 180.0 * secondPlayer.angle / M_PI,
                                 nullptr, SDL_FLIP_NONE);
            }

            for (int i = 0; i < bubbleAmount; i++) {
                auto rect = bubbleRect;
                rect.x = bubbles[i].position[0] - rect.w / 2;
                rect.y = bubbles[i].position[1] - rect.h / 2;
                SDL_RenderCopyEx(renderer, bubble_texture.get(),
                                 nullptr, &rect, 0,
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
                    howManyDigits++;
                } while (temp > 0);

                int positionX = RIGHT_SCORE_START_WIDTH + (howManyDigits - 1) * SCORE_JUMP;
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
                int timeElapsedInSeconds = difftime(currentTime, startTime);
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
                    for (int i = 0; i < 2; i++) {
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
                if (timeToEndInSeconds <= 0) {
                    gaming = 0;
                    shouldShowPreviousResult = true;
                    isGameGoingOn = false;
                }

            }

            SDL_RenderPresent(renderer);

            auto current_tick = SDL_GetTicks();
            SDL_Delay(TICK_TIME - (current_tick - prev_tick));
            prev_tick += TICK_TIME;
        }

        while (!isGameGoingOn) {
            SDL_Event sdlEvent;

            SDL_RenderCopy(renderer, background.get(), nullptr, nullptr);
            {
                auto rect = menuRect;
                rect.x = 170;
                rect.y = 100;
                SDL_RenderCopyEx(renderer, menu_texture.get(),
                                 nullptr, &rect, 0,
                                 nullptr, SDL_FLIP_NONE);
            }

            {
                auto rect = optionFrameRect;
                rect.x = 130;
                if(playerChoseMenuStartOption) {
                    rect.y = 80;
                } else {
                    rect.y = 180;
                }
                SDL_RenderCopyEx(renderer, option_frame_texture.get(),
                                 nullptr, &rect, 0,
                                 nullptr, SDL_FLIP_NONE);
            }

            while (SDL_PollEvent(&sdlEvent) != 0) {
                switch (sdlEvent.type) {
                    case SDL_QUIT:
                        Mix_FreeChunk(dustSuckSound);
                        Mix_FreeChunk(turboSound);
                        Mix_CloseAudio();
                        return;
                    case SDL_KEYDOWN:
                        if(sdlEvent.key.keysym.sym == SDLK_UP || sdlEvent.key.keysym.sym == SDLK_DOWN) {
                            playerChoseMenuStartOption = !playerChoseMenuStartOption;
                        } else if(sdlEvent.key.keysym.sym == SDLK_SPACE) {
                            if(playerChoseMenuStartOption) {
                                isGameGoingOn = true;
                                shouldShowPreviousResult = false;
                                time (&startTime);
                                firstPlayer = {0, {400.0, 200.0}};
                                secondPlayer = {0, {250.0, 200.0}};
                                gaming = true;
                                prev_tick = SDL_GetTicks();
                                ticksTillNextDustSpawn = DUST_SPAWN_TICKS;
                                ticksTillNextTurboSpawn = TURBO_SPAWN_TICKS;
                                ticksTillNextFirstPlayerBubble = 0;
                                ticksTillNextSecondPlayerBubble = 0;
                                firstPlayerDied = false;
                                secondPlayerDied = false;
                                dustAmount = 0;
                                turboAmount = 0;
                                break;
                            } else {
                                Mix_FreeChunk(dustSuckSound);
                                Mix_FreeChunk(turboSound);
                                Mix_CloseAudio();
                                return;
                            }
                        }
                }
            }

            if(shouldShowPreviousResult) {
                if (firstPlayer.points == secondPlayer.points) {
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

                if (firstPlayer.points > secondPlayer.points) {
                    auto number = 1;
                    auto rect = numberRects[number];
                    rect.x = WINNER_PLAYER_NUMBER_WIDTH;
                    rect.y = WINNER_PLAYER_NUMBER_HEIGHT + PLAYGROUND_HEIGHT + 1;
                    SDL_RenderCopyEx(renderer, numberTextures[number].get(),
                                     nullptr, &rect, 0,
                                     nullptr, SDL_FLIP_NONE);
                } else if (firstPlayer.points < secondPlayer.points) {
                    auto number = 2;
                    auto rect = numberRects[number];
                    rect.x = WINNER_PLAYER_NUMBER_WIDTH;
                    rect.y = WINNER_PLAYER_NUMBER_HEIGHT + PLAYGROUND_HEIGHT + 1;
                    SDL_RenderCopyEx(renderer, numberTextures[number].get(),
                                     nullptr, &rect, 0,
                                     nullptr, SDL_FLIP_NONE);
                }
            } else {
                auto rect = controlboardRect;

                rect.x = 0;
                rect.y = PLAYGROUND_HEIGHT + 1;
                SDL_RenderCopyEx(renderer, controlboard_texture.get(),
                                 nullptr, &rect, 0,
                                 nullptr, SDL_FLIP_NONE);
            }

            SDL_RenderPresent(renderer);
        }
    }
}

void spawnBubble(Bubble bubbles[MAX_BUBBLES_ON_MAP], int bubbleIndex, PlayerCharacter player, int playerSize,
                 int bubbleSize) {
    Bubble bubble = {};
    bubble.setPositionBehindPlayer(player, playerSize, bubbleSize);
    bubble.velocity = player.velocity * (-0.25);
    bubble.ticksTillDisappear = BUBBLE_LIFE_TICKS;
    bubbles[bubbleIndex] = bubble;
}

int clearDeadBubblesAndGetBubbleAmount(Bubble bubbles[MAX_BUBBLES_ON_MAP], int bubbleAmount) {
    for (int i = 0; i < bubbleAmount; i++) {
        if (bubbles[i].ticksTillDisappear <= 0) {
            for (int j = i; j < bubbleAmount - 1; j++) {
                bubbles[j] = bubbles[j + 1];
            }
            bubbleAmount--;
        }
    }
    return bubbleAmount;
}

bool playerScored(PlayerCharacter playerCharacter, vec2d dustPositions[MAX_DUST_AMOUNT], int dustAmount, int playerSize,
                  int dustSize) {
    for (int i = 0; i < dustAmount; i++) {
        if (playerCharacter.getDistance(dustPositions[i]) < playerSize - dustSize) {
            for (int j = i; j < dustAmount - 1; j++) {
                dustPositions[j] = dustPositions[j + 1];
            }
            return true;
        }
    }

    return false;
}

bool
playerGotTurbo(PlayerCharacter playerCharacter, vec2d turboPositions[MAX_TURBO_AMOUNT], int turboAmount, int playerSize,
               int turboWidth, int turboHeight) {
    for (int i = 0; i < turboAmount; i++) {
        if (playerCharacter.gotTurbo(turboPositions[i], turboWidth, turboHeight, playerSize)) {
            for (int j = i; j < turboAmount - 1; j++) {
                turboPositions[j] = turboPositions[j + 1];
            }
            return true;
        }
    }

    return false;
}

void spawnDust(vec2d dustPositions[MAX_DUST_AMOUNT], int dustIndex, int dustSize, int holeSize) {
    do {
        dustPositions[dustIndex] = {static_cast<double>(rand() % (PLAYGROUND_WIDTH - dustSize) + dustSize / 2),
                                    static_cast<double>(rand() % (PLAYGROUND_HEIGHT - dustSize) + dustSize / 2)};
    } while (isNewDustOnHole(dustPositions[dustIndex], holeSize));
}

bool isNewDustOnHole(vec2d dustPosition, int holeSize) {
    for(int i=0; i<HOLES_AMOUNT; i++) {
        if(getDistance(HOLES_POSITIONS[i], dustPosition) < holeSize) {
            return true;
        }
    }
    return false;
}

double getDistance(vec2d basePosition, vec2d comparedPosition) {
    return sqrt(pow((comparedPosition[0] - basePosition[0]), 2) + pow((comparedPosition[1] - basePosition[1]), 2));
}

void spawnTurbo(vec2d turboPositions[MAX_TURBO_AMOUNT], int turboIndex, int turboWidth, int turboHeight) {
    turboPositions[turboIndex] = {static_cast<double>(rand() % (PLAYGROUND_WIDTH - turboWidth) + turboWidth / 2),
                                  static_cast<double>(rand() % (PLAYGROUND_HEIGHT - turboHeight) + turboHeight / 2)};
}

bool playerFellIntoHole(PlayerCharacter playerCharacter, int playerSize, int holeSize) {
    for (int i = 0; i < HOLES_AMOUNT; i++) {
        if (playerCharacter.getDistance(HOLES_POSITIONS[i]) < holeSize - playerSize) {
            return true;
        }
    }
    return false;
}

std::pair<PlayerCharacter, PlayerCharacter>
updatePlayersIfCollision(PlayerCharacter firstPlayer, PlayerCharacter secondPlayer, PlayerCharacter firstPlayerUpdated,
                         PlayerCharacter secondPlayerUpdated, int playerSize) {
    vec2d dir = secondPlayer.position - firstPlayer.position;
    double distance = firstPlayerUpdated.getDistance(secondPlayerUpdated);
    if (distance >= playerSize) {
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
    Mix_Init(0);
    int openAudioRes = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    if(openAudioRes < 0) {
        std::cout << "Open audio failed";
    }
    int allocateChannelsRes = Mix_AllocateChannels(4);
    if( allocateChannelsRes < 0 )
    {
        fprintf(stderr, "Unable to allocate mixing channels: %s\n", SDL_GetError());
        exit(-1);
    }
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