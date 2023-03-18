//
// Created by Kasia on 18.03.2023.
//

#ifndef BALL_OF_RIVALRY_PLAYERCHARACTER_H
#define BALL_OF_RIVALRY_PLAYERCHARACTER_H

#include <array>
#include <valarray>
#include "operator_definitions.h"

using vec2d = std::array<double, 2>;

class PlayerCharacter {
public:
    double angle;
    vec2d position;
    vec2d velocity;
    vec2d acceleration;

    PlayerCharacter next_state(double dt_ms) {
        double dt = dt_ms / 1000.0;
        PlayerCharacter next = *this;
        const double C = 0.1;
        vec2d friction = {0.0,0.0};
        if (len(velocity) > 0) {
            friction = velocity*len(velocity)*C;
        }
        auto a = acceleration - friction;
        next.position = position + velocity * dt + (a * dt * dt) / 2;
        next.velocity = velocity + a * dt;
        next.acceleration = a;

        return next;
    }
};

#endif //BALL_OF_RIVALRY_PLAYERCHARACTER_H
