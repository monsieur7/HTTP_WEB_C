#include "BME280.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <math.h>

#define BME280_ADDRESS 0x76 // Adresse I2C par défaut du BME280

BME280::BME280()
{
    // Initialisation des paramètres par défaut
    address = BME280_ADDRESS;
    mode = 0b11;             // Mode de mesure normale
    standby = 0b00;          // Mode de veille désactivé
    filter = 0b000;          // Pas de filtrage
    temp_overSample = 0b101; // Oversampling x16
    humi_overSample = 0b101; // Oversampling x16
    pres_overSample = 0b101; // Oversampling x16
}

void BME280::settings(uint8_t _address, uint8_t _mode, uint8_t _standby, uint8_t _filter, uint8_t _temp_overSample, uint8_t _humi_overSample, uint8_t _pres_overSample)
{
    // Réglage des paramètres du capteur
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
    // Initialisation du capteur
    char filename[20];

    snprintf(filename, 19, "/dev/i2c-1");
    if ((_file = open(filename, O_RDWR)) < 0)
    {
        std::cerr << "Impossible d'ouvrir le bus I2C" << std::endl;
        return 1;
    }
    if (ioctl(_file, I2C_SLAVE, address) < 0)
    {
        std::cerr << "Impossible de se connecter au BME280" << std::endl;
        return 1;
    }
    reset();
    writeRegister(BME280_CTRL_HUMIDITY_REG, humi_overSample);
    writeRegister(BME280_CTRL_MEAS_REG, (temp_overSample << 5) | (pres_overSample << 2) | mode);
    writeRegister(BME280_CONFIG_REG, (standby << 5) | (filter << 2));
    return 0;
}

void BME280::reset()
{
    // Réinitialisation du BME280
    writeRegister(BME280_RST_REG, 0xB6);
    usleep(2000); // Attente de 2 ms pour la réinitialisation
}

uint8_t BME280::readRegister(uint8_t offset)
{
    // Lecture d'un registre
    uint8_t data;
    if (read(_file, &offset, 1) != 1)
    {
        std::cerr << "Échec de la lecture du registre du BME280" << std::endl;
        return 0;
    }
    return data;
}

void BME280::writeRegister(uint8_t offset, uint8_t data)
{
    // Écriture dans un registre

    uint8_t buffer[2] = {offset, data};
    if (write(_file, buffer, 2) != 2)
    {
        std::cerr << "Échec de l'écriture dans le registre du BME280" << std::endl;
        return;
    }
}

float BME280::readTemp()
{
    // Lecture de la température depuis le BME280
    int32_t adc_T = readRegisterInt16(BME280_TEMPERATURE_MSB_REG);
    adc_T <<= 8;
    adc_T |= readRegister(BME280_TEMPERATURE_LSB_REG);
    adc_T <<= 8;
    adc_T |= readRegister(BME280_TEMPERATURE_XLSB_REG);
    adc_T >>= 4;

    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0;
}

float BME280::readPressure()
{
    // Lecture de la pression depuis le BME280
    int32_t adc_P = readRegisterInt16(BME280_PRESSURE_MSB_REG);
    adc_P <<= 8;
    adc_P |= readRegister(BME280_PRESSURE_LSB_REG);
    adc_P <<= 8;
    adc_P |= readRegister(BME280_PRESSURE_XLSB_REG);
    adc_P >>= 4;

    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0; // Éviter la division par zéro
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (float)p / 256.0;
}

float BME280::readAltitude()
{
    // Calcul de l'altitude à partir de la pression atmosphérique
    float P = readPressure() / 100.0;                // Convertir la pression en hPa
    return 44330 * (1.0 - pow(P / 1013.25, 0.1903)); // Calcul de l'altitude en mètres
}

float BME280::readHumidity()
{
    // Lecture de l'humidité depuis le BME280
    int32_t adc_H = readRegisterInt16(BME280_HUMIDITY_MSB_REG);
    adc_H <<= 8;
    adc_H |= readRegister(BME280_HUMIDITY_LSB_REG);

    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - (((int32_t)calib.dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >>
                  15) *
                 (((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + ((int32_t)32768))) >>
                     10) +
                    ((int32_t)2097152)) *
                       ((int32_t)calib.dig_H2) +
                   8192) >>
                  14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    return (v_x1_u32r >> 12) / 1024.0;
}

int16_t BME280::readRegisterInt16(uint8_t offset)
{
    // Lecture d'un registre de 16 bits
    uint8_t buffer[2];
    if (read(_file, buffer, 2) != 2)
    {
        std::cerr << "Échec de la lecture du registre du BME280" << std::endl;
        return 0;
    }
    return (buffer[0] << 8) | buffer[1];
}
