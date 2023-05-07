//
// Created by Kasia on 07.05.2023.
//

#ifndef BALL_OF_RIVALRY_BUBBLE_H
#define BALL_OF_RIVALRY_BUBBLE_H
#include "operator_definitions.h"
#include "PlayerCharacter.h"

class Bubble {
public:
    vec2d position;
    vec2d velocity;
    int ticksTillDisappear;

    Bubble next_state(double dt_ms) {
        double dt = dt_ms / 1000.0;
        Bubble next = *this;
        next.position = position + velocity * dt;
        next.velocity = velocity;
        next.ticksTillDisappear = ticksTillDisappear - 1;

        return next;
    }

    void setPositionBehindPlayer(PlayerCharacter player, int playerSize, int bubbleSize) {
        vec2d playerVector = angle_to_vector(player.angle);
        position = player.position;
        position[0] = position[0] + playerSize/2;
        position[1] = position[1] + playerSize/2;
        position = position - playerVector * playerSize;
    }

private:
    vec2d angle_to_vector(double angle) {
        return {std::cos(angle), std::sin(angle)};
    }
};


#endif //BALL_OF_RIVALRY_BUBBLE_H
