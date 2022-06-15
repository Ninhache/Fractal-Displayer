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
    this->pallet_names = {};
    this->pallet_colors = {};
}

void ColorPalet::registerPallet(std::string label, std::vector<sf::Glsl::Vec4> pallet) {
    //this->pallet_pairs.insert(std::pair<std::string, std::vector<sf::Glsl::Vec4>>(label, pallet));
    this->pallet_names.push_back(label);
    this->pallet_colors.push_back(pallet);
}

std::vector<sf::Glsl::Vec4> ColorPalet::getPallet(std::string label) {
    //return this->pallet_pairs[index].second;
    //return this->pallet_pairs.at(label);
    // get index of the label in the pallet_names vector
    size_t index = std::find(this->pallet_names.begin(), this->pallet_names.end(), label) - this->pallet_names.begin();
    return this->pallet_colors[index];
}

std::vector<sf::Glsl::Vec4> ColorPalet::getPallet(size_t index) {
    return this->pallet_colors[index];
}

