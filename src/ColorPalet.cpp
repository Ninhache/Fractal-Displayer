#include "ColorPalet.hpp"

ColorPalet::ColorPalet() {
    //std::vector<std::pair<std::string, std::vector<sf::Glsl::Vec4>>> pallet_pairs = {};
    std::map<std::string, std::vector<sf::Glsl::Vec4>> pallet_pairs = {};
}

void ColorPalet::registerPallet(std::string label, std::vector<sf::Glsl::Vec4> pallet) {
    this->pallet_pairs.insert(std::pair<std::string, std::vector<sf::Glsl::Vec4>>(label, pallet));
}

std::vector<sf::Glsl::Vec4> ColorPalet::getPallet(int index) {
    return this->pallet_pairs[index].second;
}

