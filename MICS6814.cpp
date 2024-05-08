#include "MICS6814.hpp"
#include "ADS1015.hpp"
#include <iostream>
#include <bitset>

MICS6814::MICS6814()
{
    _ads1015 = ADS1015(0x49);
    // config :
    _config = {0};
    _config.reg = CONFIG_REGISTER_MODE_CONTINUOUS |
                  CONFIG_REGISTER_OS_ON | CONFIG_REGISTER_MUX_AIN0_GND |
                  CONFIG_REGISTER_PGA_6144V | CONFIG_REGISTER_DR_1600SPS |
                  CONFIG_REGISTER_COMP_QUE_DISABLE;
    _ads1015.setConfig(_config);

    std::cerr << "Written config ADC " << std::bitset<16>(_config.reg) << std::endl;
}

float MICS6814::readOxydising()
{
    _config.reg = (_config.reg & ~CONFIG_REGISTER_MUX_MASK) | CONFIG_REGISTER_MUX_AIN0_GND;
    _ads1015.setConfig(_config);
    float oxydising = _ads1015.readVoltage();
    try
    {
        oxydising = (oxydising * 56000) / (3.3 - oxydising);
    }
    catch (const std::exception &e)
    {
        oxydising = 0;
    }

    return oxydising;
}

float MICS6814::readReducing()
{
    _config.reg = (_config.reg & ~CONFIG_REGISTER_MUX_MASK) | CONFIG_REGISTER_MUX_AIN1_GND;
    _ads1015.setConfig(_config);
    float reducing = _ads1015.readVoltage();
    try
    {
        reducing = (reducing * 56000) / (3.3 - reducing);
    }
    catch (const std::exception &e)
    {
        reducing = 0;
    }

    return reducing;
}

float MICS6814::readNH3()
{
    _config.reg = (_config.reg & ~CONFIG_REGISTER_MUX_MASK) | CONFIG_REGISTER_MUX_AIN2_GND;
    _ads1015.setConfig(_config);
    float nh3 = _ads1015.readVoltage();
    try
    {
        nh3 = (nh3 * 56000) / (3.3 - nh3);
    }
    catch (const std::exception &e)
    {
        nh3 = 0;
    }

    return nh3;
}
