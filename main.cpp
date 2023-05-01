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

#include <SDL.h>
#include "operator_definitions.h"

const int TICK_TIME = 33;

std::pair<PlayerCharacter, PlayerCharacter> updatePlayersIfCollision(PlayerCharacter firstPlayer, PlayerCharacter secondPlayer,
                                                                     PlayerCharacter firstPlayerUpdated, PlayerCharacter secondPlayerUpdated, int playerSize);

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
    auto stop_texture = load_texture(renderer, "stop.bmp");
    SDL_Rect firstPlayerRect = get_texture_rect(firstPlayerTexture);
    SDL_Rect secondPlayerRect = get_texture_rect(secondPlayerTexture);
    auto playerSize = firstPlayerRect.w;
    PlayerCharacter firstPlayer = {0, {300.0, 200.0}};
    PlayerCharacter secondPlayer = {0, {400.0, 200.0}};
    int gaming = true;
    auto prev_tick = SDL_GetTicks();
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

            firstPlayer.acceleration = acceleration_vector_from_keyboard_and_player(firstPlayer, SDL_SCANCODE_UP,
                                                                                    SDL_SCANCODE_DOWN);
            firstPlayer.angle = angle_from_keyboard_and_player(firstPlayer, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
            PlayerCharacter firstPlayerUpdated = firstPlayer.next_state(TICK_TIME);

            secondPlayer.acceleration = acceleration_vector_from_keyboard_and_player(secondPlayer, SDL_SCANCODE_W,
                                                                                     SDL_SCANCODE_S);
            secondPlayer.angle = angle_from_keyboard_and_player(secondPlayer, SDL_SCANCODE_A, SDL_SCANCODE_D);
            PlayerCharacter secondPlayerUpdated = secondPlayer.next_state(TICK_TIME);

            auto players = updatePlayersIfCollision(firstPlayer, secondPlayer, firstPlayerUpdated, secondPlayerUpdated, playerSize);
            firstPlayer = players.first;
            secondPlayer = players.second;
        }

        SDL_RenderCopy(renderer, background.get(), nullptr, nullptr);

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
        SDL_RenderPresent(renderer);
        auto current_tick = SDL_GetTicks();
        SDL_Delay(TICK_TIME - (current_tick - prev_tick));
        prev_tick += TICK_TIME;
    }
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

int main() {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(640, 480,
                                SDL_WINDOW_SHOWN,
                                &window, &renderer);

    play_the_game(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}