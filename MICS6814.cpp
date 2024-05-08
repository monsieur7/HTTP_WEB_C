#include "MICS6814.hpp"
#include <iostream>
#include <bitset>

MICS6814::MICS6814()
{
    ADS1015 ads1015;
    // config :
    CONFIG_REGISTER config = {0};
    config.reg = CONFIG_REGISTER_MODE_CONTINUOUS |
                 CONFIG_REGISTER_OS_ON | CONFIG_REGISTER_MUX_AIN0_GND |
                 CONFIG_REGISTER_PGA_6144V | CONFIG_REGISTER_DR_1600SPS |
                 CONFIG_REGISTER_COMP_QUE_DISABLE;
    ads1015.setConfig(config);

    std::cerr << "Written config ADC " << std::bitset<16>(config.reg) << std::endl;
}

float readOxydising()
{
    config.reg = (config.reg & ~CONFIG_REGISTER_MUX_MASK) | CONFIG_REGISTER_MUX_AIN0_GND;
    float oxydising = ads1015.readVoltage();
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

float readReducing()
{
    config.reg = (config.reg & ~CONFIG_REGISTER_MUX_MASK) | CONFIG_REGISTER_MUX_AIN1_GND;
    ads1015.setConfig(config);
    float reducing = ads1015.readVoltage();
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

float readNH3()
{
    config.reg = (config.reg & ~CONFIG_REGISTER_MUX_MASK) | CONFIG_REGISTER_MUX_AIN2_GND;
    ads1015.setConfig(config);
    float nh3 = ads1015.readVoltage();
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
