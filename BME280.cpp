#include "BME280.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <math.h>

#define BME280_ADDRESS 0x76 // Default I2C address of the BME280

BME280::BME280()
{
    // Initialize default parameters
    address = BME280_ADDRESS;
    mode = 0b11;             // Normal mode of measurement
    standby = 0b00;          // Standby mode disabled
    filter = 0b000;          // No filtering
    temp_overSample = 0b101; // Temperature oversampling x16
    humi_overSample = 0b101; // Humidity oversampling x16
    pres_overSample = 0b101; // Pressure oversampling x16
}

void BME280::settings(uint8_t _address, uint8_t _mode, uint8_t _standby, uint8_t _filter, uint8_t _temp_overSample, uint8_t _humi_overSample, uint8_t _pres_overSample)
{
    // Set sensor parameters
    address = _address;
    mode = _mode;
    standby = _standby;
    filter = _filter;
    temp_overSample = _temp_overSample;
    humi_overSample = _humi_overSample;
    pres_overSample = _pres_overSample;
}

uint8_t BME280::begin()
{

    // Initialize the sensor
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-1");
    if ((_file = open(filename, O_RDWR)) < 0)
    {
        std::cerr << "Failed to open I2C bus" << std::endl;
        return 1;
    }
    if (ioctl(_file, I2C_SLAVE, address) < 0)
    {
        std::cerr << "Failed to connect to BME280" << std::endl;
        return 1;
    }
    calibration.dig_T1 = ((uint16_t)((readRegister(BME280_DIG_T1_MSB_REG) << 8) + readRegister(BME280_DIG_T1_LSB_REG)));
    calibration.dig_T2 = ((int16_t)((readRegister(BME280_DIG_T2_MSB_REG) << 8) + readRegister(BME280_DIG_T2_LSB_REG)));
    calibration.dig_T3 = ((int16_t)((readRegister(BME280_DIG_T3_MSB_REG) << 8) + readRegister(BME280_DIG_T3_LSB_REG)));

    calibration.dig_P1 = ((uint16_t)((readRegister(BME280_DIG_P1_MSB_REG) << 8) + readRegister(BME280_DIG_P1_LSB_REG)));
    calibration.dig_P2 = ((int16_t)((readRegister(BME280_DIG_P2_MSB_REG) << 8) + readRegister(BME280_DIG_P2_LSB_REG)));
    calibration.dig_P3 = ((int16_t)((readRegister(BME280_DIG_P3_MSB_REG) << 8) + readRegister(BME280_DIG_P3_LSB_REG)));
    calibration.dig_P4 = ((int16_t)((readRegister(BME280_DIG_P4_MSB_REG) << 8) + readRegister(BME280_DIG_P4_LSB_REG)));
    calibration.dig_P5 = ((int16_t)((readRegister(BME280_DIG_P5_MSB_REG) << 8) + readRegister(BME280_DIG_P5_LSB_REG)));
    calibration.dig_P6 = ((int16_t)((readRegister(BME280_DIG_P6_MSB_REG) << 8) + readRegister(BME280_DIG_P6_LSB_REG)));
    calibration.dig_P7 = ((int16_t)((readRegister(BME280_DIG_P7_MSB_REG) << 8) + readRegister(BME280_DIG_P7_LSB_REG)));
    calibration.dig_P8 = ((int16_t)((readRegister(BME280_DIG_P8_MSB_REG) << 8) + readRegister(BME280_DIG_P8_LSB_REG)));
    calibration.dig_P9 = ((int16_t)((readRegister(BME280_DIG_P9_MSB_REG) << 8) + readRegister(BME280_DIG_P9_LSB_REG)));

    calibration.dig_H1 = ((uint8_t)(readRegister(BME280_DIG_H1_REG)));
    calibration.dig_H2 = ((int16_t)((readRegister(BME280_DIG_H2_MSB_REG) << 8) + readRegister(BME280_DIG_H2_LSB_REG)));
    calibration.dig_H3 = ((uint8_t)(readRegister(BME280_DIG_H3_REG)));
    calibration.dig_H4 = ((int16_t)((readRegister(BME280_DIG_H4_MSB_REG) << 4) + (readRegister(BME280_DIG_H4_LSB_REG) & 0x0F)));
    calibration.dig_H5 = ((int16_t)((readRegister(BME280_DIG_H5_MSB_REG) << 4) + ((readRegister(BME280_DIG_H4_LSB_REG) >> 4) & 0x0F)));
    calibration.dig_H6 = ((uint8_t)readRegister(BME280_DIG_H6_REG));

    // See https://github.com/SolderedElectronics/BME280-Arduino-Library/blob/master/BME280.cpp
    uint8_t dataToWrite = 0;
    // Set the oversampling control words.
    // config will only be writeable in sleep mode, so first insure that.
    writeRegister(BME280_CTRL_MEAS_REG, 0x00);

    // Set the config word
    dataToWrite = (standby << 0x5) & 0xE0;
    dataToWrite |= (filter << 0x02) & 0x1C;
    writeRegister(BME280_CONFIG_REG, dataToWrite);

    // Set ctrl_hum first, then ctrl_meas to activate ctrl_hum
    dataToWrite = humi_overSample & 0x07; // all other bits can be ignored
    writeRegister(BME280_CTRL_HUMIDITY_REG, dataToWrite);

    // set ctrl_meas
    // First, set temp oversampling
    dataToWrite = (temp_overSample << 0x5) & 0xE0;
    // Next, pressure oversampling
    dataToWrite |= (pres_overSample << 0x02) & 0x1C;
    // Last, set mode
    dataToWrite |= (mode) & 0x03;
    // Load the byte
    writeRegister(BME280_CTRL_MEAS_REG, dataToWrite);
    usleep(20 * 1000); // 20ms delay
    return 0;
    if (readRegister(BME280_CHIP_ID_REG) != 0x60)
    {
        std::cerr << "Failed to find BME280" << std::endl;
        return 1;
    }
}

uint8_t BME280::readRegister(uint8_t offset)
{
    // Read a register
    uint8_t data[1];
    uint8_t address[1] = {offset};
    if (write(_file, address, 1) != 1)
    {
        std::cerr << "Failed to read BME280 register" << std::endl;
        return 0;
    }
    // Read register data
    if (read(_file, data, 1) != 1)
    {
        std::cerr << "Failed to read BME280 register" << std::endl;
        return 0;
    }
    return 0 [data];
}

void BME280::writeRegister(uint8_t offset, uint8_t data)
{
    // Write to a register

    uint8_t buffer[2] = {offset, data};
    if (write(_file, buffer, 2) != 2)
    {
        std::cerr << "Failed to write to BME280 register" << std::endl;
        return;
    }
}

float BME280::readTemp()
{
    // Read temperature from BME280
    int32_t adc_T = ((uint32_t)readRegister(BME280_TEMPERATURE_MSB_REG) << 12) | ((uint32_t)readRegister(BME280_TEMPERATURE_LSB_REG) << 4) | ((readRegister(BME280_TEMPERATURE_XLSB_REG) >> 4) & 0x0F);

    int64_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calibration.dig_T1 << 1))) * ((int32_t)calibration.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calibration.dig_T1)) * ((adc_T >> 4) - ((int32_t)calibration.dig_T1))) >> 12) * ((int32_t)calibration.dig_T3)) >> 14;
    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0;
}

float BME280::readPressure()
{
    // Read pressure from BME280 (in Pa)
    int32_t adc_P = ((uint32_t)readRegister(BME280_PRESSURE_MSB_REG) << 12) | ((uint32_t)readRegister(BME280_PRESSURE_LSB_REG) << 4) | ((readRegister(BME280_PRESSURE_XLSB_REG) >> 4) & 0x0F);

    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calibration.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calibration.dig_P5) << 17);
    var2 = var2 + (((int64_t)calibration.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calibration.dig_P3) >> 8) + ((var1 * (int64_t)calibration.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calibration.dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0; // Avoid division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calibration.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calibration.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calibration.dig_P7) << 4);
    return (float)p / 256.0;
}

float BME280::readAltitude(float QNH)
{
    // QNH : 1020 hPa
    // https://metar-taf.com/LPFR
    //  Calculate altitude from atmospheric pressure
    float P = readPressure() / 100.0f;               // Convert pressure to hPa
    return 44330 * (1.0 - pow(P / QNH, 1 / 5.255f)); // Calculate altitude in meters
    // see https://community.bosch-sensortec.com/t5/Question-and-answers/How-to-calculate-the-altitude-from-the-pressure-sensor-data/qaq-p/5702
}

float BME280::readHumidity()
{
    // Read humidity from BME280
    int32_t adc_H = ((uint32_t)readRegister(BME280_HUMIDITY_MSB_REG) << 8) | ((uint32_t)readRegister(BME280_HUMIDITY_LSB_REG));

    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calibration.dig_H4) << 20) - (((int32_t)calibration.dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >>
                  15) *
                 (((((((v_x1_u32r * ((int32_t)calibration.dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)calibration.dig_H3)) >> 11) + ((int32_t)32768))) >>
                     10) +
                    ((int32_t)2097152)) *
                       ((int32_t)calibration.dig_H2) +
                   8192) >>
                  14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calibration.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    return (v_x1_u32r >> 12) / 1024.0;
}

int16_t BME280::readRegisterInt16(uint8_t offset)
{
    uint8_t msb = readRegister(offset);
    uint8_t lsb = readRegister(offset + 1);

    return (int16_t)((msb << 8) | lsb);
}
