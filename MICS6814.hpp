#include "ADS1015.hpp"
#include <cstdint>

class MICS6814
{
public:
    MICS6814();
    float readOxydising();
    float readNH3();
    float readReducing();
    
private:

};