#ifndef COLORPALET_HPP
#define COLORPALET_HPP

#include <SFML/Graphics.hpp>
#include <iostream>

sf::Glsl::Vec4* palletToArray(std::vector<sf::Glsl::Vec4> pallet);

class ColorPalet {
    public:
        ColorPalet();
        
        //std::vector<std::pair<std::string, std::vector<std::vector<sf::Glsl::Vec4>>>> pallet_pairs;
        std::map<std::string, std::vector<sf::Glsl::Vec4>> pallet_pairs;
        std::vector<std::string> pallet_names;
        std::vector<std::vector<sf::Glsl::Vec4>> pallet_colors;
        sf::Glsl::Vec4 background_color;

        std::vector<sf::Glsl::Vec4> getPallet(std::string label);
        std::vector<sf::Glsl::Vec4> getPallet(size_t index);
        
        void setBackgroundColor(sf::Glsl::Vec4 color);        
        void registerPallet(std::string label, std::vector<sf::Glsl::Vec4> pallet);
        //std::vector<std::vector<sf::Glsl::Vec4>> getPalletByLabel(std::string label);
};
#endif