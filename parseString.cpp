#include "parseString.hpp"
#include <iostream>

ParseString::ParseString(std::string data) : data_(data) {}

void ParseString::parseHTML() {
    // Liste pour stocker les balises rencontrées
    std::vector<std::string> tags;

    // Recherche des balises ouvrantes et fermantes
    size_t pos = 0;
    while ((pos = data_.find('<', pos)) != std::string::npos) {
        size_t end_pos = data_.find('>', pos);
        if (end_pos != std::string::npos) {
            // Extraire la balise entre '<' et '>'
            std::string tag = data_.substr(pos + 1, end_pos - pos - 1);
            tags.push_back(tag);
            pos = end_pos + 1;
        } else {
            // Balise mal formée, passer à la position suivante
            pos++;
        }
    }

    // Affichage des balises trouvées
    std::cout << "Balises HTML trouvées : " << std::endl;
    for (const auto& tag : tags) {
        std::cout << "<" << tag << ">" << std::endl;
    }
}