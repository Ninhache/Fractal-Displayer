#include "Fractal.hpp"


Fractal::Fractal(Type type) {
    this->type = type;
    this->iterations = 100;
    this->scale = 1.0f;
    this->center = sf::Glsl::Vec2(0.0f, 0.0f);
    this->pallet = GLOBAL_PALLET.getPallet((size_t)0);
}

Fractal::Fractal(int type) {
    this->type = (Type)type;
    this->iterations = 100;
    this->scale = 1.0f;
    this->center = sf::Glsl::Vec2(0.0f, 0.0f);
    this->pallet = GLOBAL_PALLET.getPallet((size_t)0);
}

Fractal::Fractal() {
    this->type = Type::Mandelbrot;
    this->iterations = 100;
    this->scale = 1.0f;
    this->center = sf::Glsl::Vec2(0.0f, 0.0f);
    this->pallet = GLOBAL_PALLET.getPallet((size_t)0);
}