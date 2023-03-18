//
// Created by Kasia on 18.03.2023.
//

#ifndef BALL_OF_RIVALRY_OPERATOR_DEFINITIONS_H
#define BALL_OF_RIVALRY_OPERATOR_DEFINITIONS_H

#include <array>
#include <valarray>

using vec2d = std::array<double, 2>;

vec2d operator+(vec2d a, vec2d b) {
    return {a[0] + b[0], a[1] + b[1]};
}

vec2d operator-(vec2d a, vec2d b) {
    return {a[0] - b[0], a[1] - b[1]};
}

vec2d operator*(vec2d a, double b) {
    return {a[0] * b, a[1] * b};
}

vec2d operator/(vec2d a, double b) {
    return a * (1.0 / b);
}
double len(vec2d v) {
    return std::sqrt(v[0]*v[0] + v[1]*v[1]);
}
class operator_definitions {

};


#endif //BALL_OF_RIVALRY_OPERATOR_DEFINITIONS_H
