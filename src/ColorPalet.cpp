#include "ColorPalet.hpp"

sf::Glsl::Vec4* palletToArray(std::vector<sf::Glsl::Vec4> pallet) {
    sf::Glsl::Vec4* array = new sf::Glsl::Vec4[pallet.size()];
    for (size_t i = 0; i < pallet.size(); i++) {
        array[i] = pallet[i];
    }
    return array;
}

ColorPalet::ColorPalet() {
    //std::vector<std::pair<std::string, std::vector<sf::Glsl::Vec4>>> pallet_pairs = {};
    this->pallet_pairs = {};
}

void ColorPalet::registerPallet(std::string label, std::vector<sf::Glsl::Vec4> pallet) {
    this->pallet_pairs.insert(std::pair<std::string, std::vector<sf::Glsl::Vec4>>(label, pallet));
}

std::vector<sf::Glsl::Vec4> ColorPalet::getPallet(std::string label) {
    //return this->pallet_pairs[index].second;
    return this->pallet_pairs.at(label);
}

std::vector<sf::Glsl::Vec4> ColorPalet::getPallet(size_t index) {
    if (this->pallet_pairs.size() > index) {
        size_t i = 0;
        for (auto const& [key, val] : this->pallet_pairs) {
            if (i == index) {
                return val;
            }
            i++;
        }
    } else {
        std::cout << "Error: ColorPalet::getPallet(int index) - index out of range" << std::endl;
        return std::vector<sf::Glsl::Vec4>();
    }
}

