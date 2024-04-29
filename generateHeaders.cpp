#include "generateHeaders.hpp"

generateHeaders::generateHeaders(std::map<std::string, std::string> headers) : _headers(headers) {}
std::string generateHeaders::generateResponse(int error_code)

{
    std::string response = "HTTP/1.1 " + std::to_string(error_code) + " OK\r\n";
    for (auto const &header : _headers)
    {
        if (header.first == "Method" || header.first == "Path" || header.first == "HTTP Version")
            continue;
        else
            response += header.first + ": " + header.second + "\r\n";
    }
    response += "\r\n";
    return response;
}