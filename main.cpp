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

vec2d acceleration_vector_from_keyboard_and_player(const PlayerCharacter &player) {
    auto *keyboard_state = SDL_GetKeyboardState(nullptr);
    vec2d forward_vec = angle_to_vector(player.angle);
    vec2d acceleration = {0, 0};
    if (keyboard_state[SDL_SCANCODE_UP]) {
        acceleration = acceleration + forward_vec;
    }
    if (keyboard_state[SDL_SCANCODE_DOWN]) {
        acceleration = acceleration - forward_vec;
    }
    return acceleration*1500.0;
}

double angle_from_keyboard_and_player(const PlayerCharacter &player) {
    auto *keyboard_state = SDL_GetKeyboardState(nullptr);
    double angle = player.angle;
    if (keyboard_state[SDL_SCANCODE_LEFT]) angle = angle - M_PI / 10.0;
    if (keyboard_state[SDL_SCANCODE_RIGHT]) angle = angle + M_PI / 10.0;
    return angle;
}

void play_the_game(SDL_Renderer *renderer) {
    auto player_texture = load_texture(renderer, "player1.bmp");
    auto background = load_texture(renderer, "wooden_floor.bmp");
    auto stop_texture = load_texture(renderer, "stop.bmp");
    SDL_Rect player_rect = get_texture_rect(player_texture);
    PlayerCharacter player = {0, {320.0, 200.0}};
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

        player.acceleration = acceleration_vector_from_keyboard_and_player(player);
        player.angle = angle_from_keyboard_and_player(player);
        player = player.next_state(TICK_TIME);


        SDL_RenderCopy(renderer, background.get(), nullptr, nullptr);

        {
            auto rect = player_rect;

            rect.x = player.position[0] - rect.w / 2;
            rect.y = player.position[1] - rect.h / 2;
            SDL_RenderCopyEx(renderer, player_texture.get(),
                             nullptr, &rect, 180.0 * player.angle / M_PI,
                             nullptr, SDL_FLIP_NONE);
        }
        SDL_RenderPresent(renderer);
        auto current_tick = SDL_GetTicks();
        SDL_Delay(TICK_TIME - (current_tick - prev_tick));
        prev_tick += TICK_TIME;
    }
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