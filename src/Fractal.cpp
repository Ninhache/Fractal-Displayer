#include "Fractal.hpp"


Fractal::Fractal(Type type) {
    this->type = type;
    this->iterations = 100;
    this->scale = 1.0f;
    this->center = sf::Glsl::Vec2(0.0f, 0.0f);
    this->pallet = rgb_pallete;
}