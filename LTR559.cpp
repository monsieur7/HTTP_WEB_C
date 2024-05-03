#include "LTR559.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#define LTR559_ADDRESS 0x23 

LTR559::LTR559() {
    char filename[20];
    int adapter_nr = 1; // I2C bus 1
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, O_RDWR);
    if (file < 0) {
        std::cerr << "Failed to open the bus." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    if (ioctl(file, I2C_SLAVE, LTR559_ADDRESS) < 0) {
        std::cerr << "Failed to acquire bus access and/or talk to slave." << std::endl;
        // Throw an exception or handle the error appropriately
    }
    usleep(100000);

    uint8_t partNumber = readRegister(LTR559_PART_ID);

    writeRegister(LTR559_ALS_CONTROL,0x03);
    usleep(10000);
    writeRegister(LTR559_PS_CONTROL,LTR559_PS_CONTROL_ACTIVE_MASK);
    
}

void LTR559::writeRegister(uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = {reg, data};
    if (write(file, buffer, 2) != 2) {
        std::cerr << "Error writing to i2c slave" << std::endl;
        // Throw an exception or handle the error appropriately
    }
}

uint8_t LTR559::readRegister(uint8_t reg) {
    if (write(file, &reg, 1) != 1) {
        std::cerr << "Error writing to i2c slave" << std::endl;
        // Throw an exception or handle the error appropriately
    }
    uint8_t data;
    if (read(file, &data, 1) != 1) {
        std::cerr << "Error reading from i2c slave" << std::endl;
        // Throw an exception or handle the error appropriately
    }
    return data;
}

int32_t LTR559::getLux() { 
   int als1 = readRegisterInt16(LTR559_ALS_DATA_CH1);
   int als0 =readRegisterInt16(LTR559_ALS_DATA_CH0);
   int ps0 = readRegisterInt16(LTR559_PS_DATA);

   return (int32_t)(als1 << 8 | als0);


}

int16_t LTR559::readRegisterInt16(uint8_t offset) {
    uint8_t msb = readRegister(offset);
    uint8_t lsb = readRegister(offset + 1);

    return (int16_t)((msb << 8) | lsb);
}
