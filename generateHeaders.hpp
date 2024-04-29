#include <string>
#include <map>

class generateHeaders
{
private:
    std::map<std::string, std::string> _headers;

public:
    generateHeaders(std::map<std::string, std::string> headers);
    std::string generateResponse(int error_code = 200);
};
