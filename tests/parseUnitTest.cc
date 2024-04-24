#include <gtest/gtest.h>
#include "../parseString.hpp"
TEST(ParseString, parseHeader)
{
    std::string input = "Première ligne\r\nDeuxième ligne\r\nTroisième ligne\r\n";
    ParseString data(input);
    std::vector<std::string> lines = data.parseHeader();

    ASSERT_EQ(lines.size(), 4);
    ASSERT_EQ(lines[0], "Première ligne");
    ASSERT_EQ(lines[1], "Deuxième ligne");
    ASSERT_EQ(lines[2], "Troisième ligne");
}
