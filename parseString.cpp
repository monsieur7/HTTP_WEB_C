#include "parseString.hpp"
#include <iostream>

ParseString::ParseString(std::string data) : data_(data) {}

std::vector<std::string> ParseString::parseHeader()
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = 0;

    while ((end = data_.find("\r\n", start)) != std::string::npos)
    {
        result.push_back(data_.substr(start, end - start));
        start = end + 2; // On avance de 2 pour sauter "\r\n"
    }

    // Ajouter la dernière sous-chaîne
    result.push_back(data_.substr(start));

    return result;
}