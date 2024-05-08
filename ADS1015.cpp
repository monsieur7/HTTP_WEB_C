#include "ADS1015.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <bitset>
#include <cstring>
// TODO : set address before all the operations
ADS1015::ADS1015(uint8_t address)
{
    _address = address;
    char filename[20];
    int adapter_nr = 1; // I2C bus 1
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    _file = open(filename, O_RDWR);
    if (_file < 0)
    {
        std::cerr << "ADC Failed to open the bus." << std::endl;
        std::cerr << strerror(errno) << std::endl;

        // Throw an exception or handle the error appropriately
    }
    if (ioctl(_file, I2C_SLAVE, address) < 0)
    {
        std::cerr << "ADC Failed to acquire bus access and/or talk to slave." << std::endl;
        std::cerr << strerror(errno) << std::endl;

        // Throw an exception or handle the error appropriately
    }
}

void ADS1015::init()
{
    _config.reg = CONFIG_REGISTER_MODE_CONTINUOUS |
                  CONFIG_REGISTER_OS_ON | CONFIG_REGISTER_MUX_AIN0_GND |
                  CONFIG_REGISTER_PGA_6144V | CONFIG_REGISTER_DR_1600SPS |
                  CONFIG_REGISTER_COMP_QUE_DISABLE;
    setConfig(_config);
}

void ADS1015::writeRegister(uint8_t reg, uint16_t value)
{

    if (ioctl(_file, I2C_SLAVE, _address) < 0)
    {
        std::cerr << "ADC Failed to acquire bus access and/or talk to slave." << std::endl;
        std::cerr << strerror(errno) << std::endl;

        // Throw an exception or handle the error appropriately
    }
    // write address and data:
    uint8_t data[] = {reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    if (write(_file, data, 3) != 3)
    {
        std::cerr << "ADC Failed to write to the i2c bus." << std::endl;
        std::cerr << strerror(errno) << std::endl;
        // Throw an exception or handle the error appropriately
    }
}
uint16_t ADS1015::readRegister(uint8_t reg)
{
    if (ioctl(_file, I2C_SLAVE, _address) < 0)
    {
        std::cerr << "ADC Failed to acquire bus access and/or talk to slave." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    // write address :
    uint8_t addr[] = {reg};
    if (write(_file, addr, 1) != 1)
    {
        std::cerr << "ADC Failed to write to the i2c bus." << std::endl;
        std::cerr << strerror(errno) << std::endl;

        // Throw an exception or handle the error appropriately
    }
    // read value :
    uint8_t buf[2];
    if (read(_file, buf, 2) != 2)
    {
        std::cerr << "ADC Failed to read from the i2c bus." << std::endl;
        std::cerr << strerror(errno) << std::endl;

        // Throw an exception or handle the error appropriately
    }
    return (buf[0] << 8 | buf[1]); // little endian
}
void ADS1015::setConfig(CONFIG_REGISTER config)
{
    writeRegister(ADS1015_CONFIG_REGISTER, config.reg);
}
uint16_t ADS1015::getConfig()
{
    return readRegister(ADS1015_CONFIG_REGISTER);
}
int16_t ADS1015::readADC()
{
    return (readRegister(ADS1015_CONVERSION_REGISTER)) >> 4;
}
float ADS1015::readVoltage(bool continuous) // in continuous mode !
{
    // dont wait for conversion
    // we are in continuous mode (expected)
    // DONT TOUCH
    if (!continuous) // if we are in single mode, wait for end of conversion
    {
        do
        {
            usleep(10);
        } while ((readRegister(ADS1015_CONFIG_REGISTER) & CONFIG_REGISTER_OS_MASK) == 0);
    }

    std::cerr << "config register : " << std::bitset<16>(readRegister(ADS1015_CONFIG_REGISTER)) << std::endl;

    int16_t adcValue = readADC();
    // get gain :
    uint16_t config = getConfig();
    uint16_t gain = (config >> CONFIG_REGISTER_PGA_OFFSET) & 0x07;
    float gainV = 0;
    float voltage = (float)adcValue;
    switch (gain)
    {
    case 0:
        gainV = 6.144;
        break;
    case 1:
        gainV = 4.096;
        break;
    case 2:
        gainV = 2.048;
        break;
    case 3:
        gainV = 1.024;
        break;
    case 4:
        gainV = 0.512;
        break;
    case 5:
        gainV = 0.256;
        break;
    default:
        gainV = 2.048;
        break;
    }
    std::cerr << "ADC gain : " << gainV << std::endl;
    return (voltage * gainV) / 2048.0f;
}

float ADS1015::readOxydising()
{
    _config.reg = (_config.reg & ~CONFIG_REGISTER_MUX_MASK) | CONFIG_REGISTER_MUX_AIN0_GND;
    setConfig(_config);
    float oxydising = readVoltage();
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
