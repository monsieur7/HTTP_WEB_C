
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <vector>
#include <string>
#include <sstream>
#include <poll.h>

class ParseString{
  private:
    std::string data_;
    void parseHTML();

  public:
    ParseString(std::string data);


};