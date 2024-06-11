#include <gtest/gtest.h>
#include "../parseString.hpp"
#include "../generateHeaders.hpp"

class ParseStringTest : public ::testing::Test // TEXT FIXTURE !
{
protected:
    ParseStringTest(std::string data) : instance(data){};
    ParseStringTest() : instance(""){};
    void SetUp() override
    {
        instance = ParseString("GET /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n");
    };
    void TearDown() override {};
    ParseString instance;
};

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
    // debug :
    std::cout << "headers.size() : " << headers.size() << std::endl;
    EXPECT_EQ(headers.size(), 6);
    EXPECT_EQ(headers["Method"], "GET");
    EXPECT_EQ(headers["Path"], "/home.html");
    EXPECT_EQ(headers["HTTP Version"], "1.1");
    EXPECT_EQ(headers["Host"], "localhost:8080");
    EXPECT_EQ(headers["User-Agent"], "Mozilla/5.0");
}

TEST_F(ParseStringTest, ParseGetter)
{
    instance.setData("GET /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n");
    EXPECT_EQ(instance.getData(), "GET /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n");
}

TEST_F(ParseStringTest, ParseSetter)
{
    instance.setData("GET /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n");
    EXPECT_EQ(instance.getData(), "GET /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n");
    instance.setData("POST /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n");
    EXPECT_EQ(instance.getData(), "POST /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n");
}

TEST(generateHeaders, GenerateResponse)
{
    std::string input = "GET /home.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n";
    ParseString data(input);
    std::map<std::string, std::string> headers = data.parseRequest();
    generateHeaders response(headers);
    std::string result = response.generateResponse();
    std::string expected = "HTTP/1.1 200 OK\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\n\r\n";
}
TEST(generateHeaders, EmptyResponse)
{
    std::map<std::string, std::string> headers;
    generateHeaders response(headers);
    std::string result = response.generateResponse();
    std::string expected = "HTTP/1.1 200 OK\r\n\r\n";
    EXPECT_EQ(result, expected);
}
