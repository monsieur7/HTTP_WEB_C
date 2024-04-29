#include <regex>
#include "parseString.hpp"

ParseString::ParseString(std::string data) : data_(data) {}

std::vector<std::string> ParseString::parseHeader()
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = 0;

    while ((end = data_.find("\r\n", start)) != std::string::npos)
    {
        result.push_back(data_.substr(start, end - start));
        start = end + 2; // Move past the "\r\n"
    }

    // Add the last substring
    result.push_back(data_.substr(start));

    return result;
}

std::map<std::string, std::string> ParseString::parseRequest()
{
    std::map<std::string, std::string> result;
    std::vector<std::string> lines = this->parseHeader();

    // Regular expression to match "key: value" pattern
    std::regex header_regex(R"(([^:]+): *(.+))");
    std::regex request_regex(R"((\S+)\s+(\S+)\s+HTTP/(\d+\.\d+))");
    // where \S matches any non-whitespace character, \s matches any whitespace character, and \d matches any digit.
    for (const auto &line : lines)
    {
        if (!line.empty())
        {
            std::smatch match;
            if (std::regex_match(line, match, header_regex))
            {
                std::string key = match[1];
                std::string value = match[2];

                // Store key-value pair in the result map
                result[key] = value;
            }
            else if (std::regex_match(line, match, request_regex))
            {
                std::string key = "Method";
                std::string value = match[1];

                result[key] = value;

                key = "Path";
                value = match[2];
                result[key] = value;

                key = "HTTP Version";
                value = match[3];
                result[key] = value;
            }
        }
    }

    return result;
}
