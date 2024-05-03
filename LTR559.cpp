#include "LTR559.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#define LTR559_ADDRESS 0x23

LTR559::LTR559()
{
    char filename[20];
    int adapter_nr = 1; // I2C bus 1
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, O_RDWR);
    if (file < 0)
    {
        std::cerr << "Failed to open the bus." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    if (ioctl(file, I2C_SLAVE, LTR559_ADDRESS) < 0)
    {
        std::cerr << "Failed to acquire bus access and/or talk to slave." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    usleep(100000);

    // setup soft reset :

    writeRegister(LTR559_INTERRUPT, 0x03);
    // wait for the bit to get back to 0
    while (readRegister(LTR559_INTERRUPT) & 0x03)
    {
        usleep(1000);
    }
    // setup interrupt :
    writeRegister(LTR559_ALS_CONTROL, 0x03);
    // PS LED SETUP :50mA / 1.0 duty cycle / 30kHz

    writeRegister(LTR559_PS_LED, (0 & 0x3) << 5 | (0b11 & 0x3) << 3 | (0b011 & 0x7));

    // 1 pulses :default

    // MEAS RATE : PS : 100ms (default)
    writeRegister(LTR559_PS_MEAS_RATE, 0b0010);
    // ALS MEAS RATE : 50ms / 50ms
    writeRegister(LTR559_ALS_MEAS_RATE, (0b001 & 0x7) << 3 | (0b000 & 0x7));

    //  ALS CONTROL :
    writeRegister(LTR559_ALS_CONTROL, 0b000_000_00);

    // threshold :
    writeRegister(LTR559_ALS_THRESHOLD_HIGH, 0xFF);
    writeRegister(LTR559_ALS_THRESHOLD_LOW, 0x00);
    // ps threshold :
    writeRegister(LTR559_PS_THRESHOLD_HIGH, 0xFF);
    writeRegister(LTR559_PS_THRESHOLD_LOW, 0x00);

    // offset : 0
    writeRegister(LTR559_OFFSET, 0x00);

    // setup ALS and PS
    // PS SETUP :
    writeRegister(LTR559_PS_CONTROL, LTR559_PS_CONTROL_ACTIVE_MASK);
    // ALS SETUP :
    writeRegister(LTR559_ALS_CONTROL, 0b000_000_01);
}

void LTR559::writeRegister(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2] = {reg, data};
    if (write(file, buffer, 2) != 2)
    {
        std::cerr << "Error writing to i2c slave" << std::endl;
        // Throw an exception or handle the error appropriately
    }
}

uint8_t LTR559::readRegister(uint8_t reg)
{
    if (write(file, &reg, 1) != 1)
    {
        std::cerr << "Error writing to i2c slave" << std::endl;
        // Throw an exception or handle the error appropriately
    }
    uint8_t data;
    if (read(file, &data, 1) != 1)
    {
        std::cerr << "Error reading from i2c slave" << std::endl;
        // Throw an exception or handle the error appropriately
    }
    return data;
}

int32_t LTR559::getLux()
{
    int16_t als1 = readRegisterInt16(LTR559_ALS_DATA_CH1);
    int16_t als0 = readRegisterInt16(LTR559_ALS_DATA_CH0);
    int16_t ps0 = readRegisterInt16(LTR559_PS_DATA);

    int32_t als = (int32_t)(als1 << 8 | als0);

    // get status :
    uint8_t status = readRegister(LTR559_ALS_PS_STATUS);
}

int16_t LTR559::readRegisterInt16(uint8_t offset)
{
    uint8_t msb = readRegister(offset);
    uint8_t lsb = readRegister(offset + 1);

    return (int16_t)((msb << 8) | lsb);
}
