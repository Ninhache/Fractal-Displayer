#ifndef COLORPALET_HPP
#define COLORPALET_HPP

#include <SFML/Graphics.hpp>
#include <iostream>

sf::Glsl::Vec4* palletToArray(std::vector<sf::Glsl::Vec4> pallet) {
    sf::Glsl::Vec4* array = new sf::Glsl::Vec4[pallet.size()];
    for (size_t i = 0; i < pallet.size(); i++) {
        array[i] = pallet[i];
    }
    return array;
}

class ColorPalet {
    public:
        ColorPalet();
        
        std::vector<std::pair<std::string, std::vector<std::vector<sf::Glsl::Vec4>>>> pallet_pairs;

        std::vector<sf::Glsl::Vec4> getPallet(int index);
        void registerPallet(std::string label, std::vector<sf::Glsl::Vec4> pallet);
        //std::vector<std::vector<sf::Glsl::Vec4>> getPalletByLabel(std::string label);
};
#endif