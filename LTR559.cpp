#include "LTR559.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <bitset>
#include <math.h>
#define LTR559_ADDRESS 0x23

LTR559::LTR559()
{
    _integrationTime = 50;
    _gain = 4;
    _lux = 0;
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
    writeRegister(LTR559_ALS_CONTROL, 0x02);
    // wait for the bit to get back to 0
    while ((readRegister(LTR559_ALS_CONTROL) >> 1) == 1)
    {
        usleep(1000);
    }
    // setup interrupt :
    writeRegister(LTR559_INTERRUPT, 0x03);
    // PS LED SETUP :50mA / 1.0 duty cycle / 30kHz

    writeRegister(LTR559_PS_LED, (0 & 0x3) << 5 | (0b11 & 0x3) << 3 | (0b011 & 0x7));

    // 1 pulses :default

    // MEAS RATE : PS : 100ms (default)
    writeRegister(LTR559_PS_MEAS_RATE, 0b0010);
    // ALS MEAS RATE : 50ms / 50ms
    writeRegister(LTR559_ALS_MEAS_RATE, (0b001 & 0x7) << 3 | (0b000 & 0x7));

    // threshold :
    writeRegister(LTR559_ALS_THRESHOLD_UPPER, 0xFF);
    writeRegister(LTR559_ALS_THRESHOLD_UPPER + 1, 0xFF);

    writeRegister(LTR559_ALS_THRESHOLD_LOWER, 0x00);
    writeRegister(LTR559_ALS_THRESHOLD_LOWER + 1, 0x00);

    // ps threshold :
    writeRegister(LTR559_PS_THRESHOLD_UPPER, 0xFF);
    writeRegister(LTR559_PS_THRESHOLD_UPPER + 1, 0x0F);

    writeRegister(LTR559_PS_THRESHOLD_LOWER, 0x00);
    writeRegister(LTR559_PS_THRESHOLD_LOWER + 1, 0x00);

    // offset : 0
    writeRegister(LTR559_PS_OFFSET, 0x00);

    // setup ALS and PS
    // PS SETUP : - start PS
    writeRegister(LTR559_PS_CONTROL, LTR559_PS_CONTROL_ACTIVE_MASK << 0);
    // ALS SETUP : - start ALS - gain 1

    writeRegister(LTR559_ALS_CONTROL, (1 << LTR559_ALS_CONTROL_MODE_BIT) | (2 << LTR559_ALS_CONTROL_GAIN_SHIFT));
    // DEBUG : dump all registers
    for (int i = 0x80; i < 0x9E; i++)
    {
        std::cerr << "Register " << std::hex << i << " : " << std::dec << std::bitset<8>(readRegister(i)) << std::endl;
    }
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

float LTR559::getLux()
{
    // Get status from ALS_PS_STATUS register
    uint8_t status = readRegister(LTR559_ALS_PS_STATUS);
    if ((status & (1 << LTR559_ALS_PS_STATUS_ALS_DATA_BIT)) == 0)
    {
        return _lux;
    }
    // check for interrupt :
    if ((status << LTR559_ALS_PS_STATUS_ALS_INTERRUPT_BIT) == 1)
    {
        std::cerr << "ALS interrupt" << std::endl;
    }
    // TODO : check if the data is valid and ready / interrupt
    //  Read ALS data registers
    uint16_t als1 = readRegisterInt16(LTR559_ALS_DATA_CH1); // see page 22 of doc
    uint16_t als0 = readRegisterInt16(LTR559_ALS_DATA_CH0);
    // Calculate ALS ratio
    float als_ratio = 0;
    // SEE https://android.googlesource.com/kernel/msm/+/android-msm-seed-3.10-lollipop-mr1/drivers/input/misc/ltr559.c

    if (als0 + als1 <= 0)
    {
        als_ratio = 101;
    }
    else
    {
        als_ratio = ((float)als1 * 100.0f) / ((float)als0 + (float)als1);
    }

    // Determine Lux Index based on ALS ratio
    int idx = 0;
    if (als_ratio < 45)
        idx = 0;
    else if (als_ratio < 64 && als_ratio >= 45)
        idx = 1;
    else if (als_ratio < 85 && als_ratio >= 64)
        idx = 2;
    else
        idx = 3;

    _lux = (_ch0_c[idx] * (float)als0 - _ch1_c[idx] * (float)als1);
    // CHECK IF LUX IS NEGATIVE OR ZERO
    if (_lux <= 0)
    {
        return _lux;
    }

    _lux = _lux / (_integrationTime / 100.0f); // integration time
    _lux = _lux / _gain;                       // gain
    _lux = _lux / 10000.0f;                    // see doc / python lib
    return _lux;
}

uint16_t LTR559::readRegisterInt16(uint8_t offset)
{
    uint8_t lsb = readRegister(offset);
    uint8_t msb = readRegister(offset + 1);
    return (uint16_t)((msb << 8) | lsb);
}
