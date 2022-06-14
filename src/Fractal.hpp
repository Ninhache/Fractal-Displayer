#ifndef FRACTAL_HPP
#define FRACTAL_HPP

#include "ColorPalet.hpp"
#include "iostream"
#include <SFML/Graphics.hpp>
#include "main.hpp"

class Fractal {

    public:
        enum Type {
            Mandelbrot = 0,
            Julia = 1
        };

        std::vector<sf::Glsl::Vec4> pallet;
        sf::Glsl::Vec2 center;
        float scale;
        int iterations;
        Type type;

        Fractal();
        Fractal(int type);
        Fractal(Type type);
        
        void setType(Type type);
        void setIterations(int iterations);
        void setScale(float scale);
        void setPallet(sf::Glsl::Vec4 pallet[], int length);
        void setCenter(sf::Glsl::Vec2 center);

        int getType();
};

#endif
