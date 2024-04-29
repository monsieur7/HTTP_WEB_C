#include <gtest/gtest.h>
#include "../parseString.hpp"
TEST(ParseString, parseHeader)
{
    std::string input = "Première ligne\r\nDeuxième ligne\r\nTroisième ligne\r\n";
    ParseString data(input);
    std::vector<std::string> lines = data.parseHeader();

    EXPECT_EQ(lines.size(), 4);
    EXPECT_EQ(lines[0], "Première ligne");
    EXPECT_EQ(lines[1], "Deuxième ligne");
    EXPECT_EQ(lines[2], "Troisième ligne");
}

TEST(ParseString, parseHeaderEmpty)
{
    std::string input = "";
    ParseString data(input);
    std::vector<std::string> lines = data.parseHeader();

    EXPECT_EQ(lines.size(), 1);
    EXPECT_EQ(lines[0], "");
}

TEST(ParseString, ParseHeaderDict)
{
    std::string input = "GET /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n";
    ParseString data(input);
    std::map<std::string, std::string> headers = data.parseRequest();
    EXPECT_EQ(headers.size(), 5);
    EXPECT_EQ(headers["Method"], "GET");
    EXPECT_EQ(headers["Path"], "/home.html");
    EXPECT_EQ(headers["HTTP Version"], "1.1");
    EXPECT_EQ(headers["Host"], "localhost:8080");
    EXPECT_EQ(headers["User-Agent"], "Mozilla/5.0");
}