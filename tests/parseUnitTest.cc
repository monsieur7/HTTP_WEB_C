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
