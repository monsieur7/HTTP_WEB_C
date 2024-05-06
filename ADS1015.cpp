#include "ADS1015.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>
ADS1015::ADS1015(uint8_t address)
{

    char filename[20];
    int adapter_nr = 1; // I2C bus 1
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    _file = open(filename, O_RDONLY);
    if (_file < 0)
    {
        std::cerr << "Failed to open the bus." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    if (ioctl(_file, I2C_SLAVE, address) < 0)
    {
        std::cerr << "Failed to acquire bus access and/or talk to slave." << std::endl;
        // Throw an exception or handle the error appropriately
    }
}

void ADS1015::writeRegister(uint8_t reg, uint16_t value)
{
    // write address :
    uint8_t addr[] = {reg};
    if (write(_file, addr, 1) != 1)
    {
        std::cerr << "Failed to write to the i2c bus." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    // write value :
    uint8_t buf[2] = {value & 0xff, (value >> 8) & 0xff}; // little endian
    if (write(_file, buf, 2) != 2)
    {
        std::cerr << "Failed to write to the i2c bus." << std::endl;
        // Throw an exception or handle the error appropriately
    }
}
uint16_t ADS1015::readRegister(uint8_t reg)
{
    // write address :
    uint8_t addr[] = {reg};
    if (write(_file, addr, 1) != 1)
    {
        std::cerr << "Failed to write to the i2c bus." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    // read value :
    uint8_t buf[2];
    if (read(_file, buf, 2) != 2)
    {
        std::cerr << "Failed to read from the i2c bus." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    return (buf[1] << 8) | buf[0]; // little endian
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
float ADS1015::readVoltage()
{
    return readADC() * (float)ADS1015_VOLTAGE_PER_BIT;
}
