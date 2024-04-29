#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <regex>
class ParseString
{
private:
  std::string data_;
  std::map<std::string, std::string> headers_;

public:
  ParseString(std::string data);
  std::vector<std::string> parseHeader();
  std::map<std::string, std::string> parseRequest();
  void setData(std::string data);
  const std::string &getData() const;
};