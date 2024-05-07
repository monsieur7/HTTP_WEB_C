// registers definition
#include <stdint.h>
typedef struct CONV_REGISTER
{
    uint16_t reg;

} CONV_REGISTER;

typedef struct CONFIG_REGISTER
{
    uint16_t reg;
} CONFIG_REGISTER;
// NOLI SE TANGERE !
#define CONFIG_REGISTER_OS_OFFSET 15
#define CONFIG_REGISTER_MUX_OFFSET 12
#define CONFIG_REGISTER_PGA_OFFSET 9
#define CONFIG_REGISTER_MODE_OFFSET 8
#define CONFIG_REGISTER_DR_OFFSET 5
#define CONFIG_REGISTER_COMP_MODE_OFFSET 4
#define CONFIG_REGISTER_COMP_POL_OFFSET 3
#define CONFIG_REGISTER_COMP_LAT_OFFSET 2
#define CONFIG_REGISTER_COMP_QUE_OFFSET 0

#define CONFIG_REGISTER_OS_ON (1 << CONFIG_REGISTER_OS_OFFSET)
#define CONFIG_REGISTER_OS_OFF (0 << CONFIG_REGISTER_OS_OFFSET)
#define CONFIG_REGISTER_OS_MASK (1 << CONFIG_REGISTER_OS_OFFSET)
#define CONFIG_REGISTER_MUX_MASK 0x7000
#define CONFIG_REGISTER_MUX_AIN0_AIN1 (0x00 << CONFIG_REGISTER_MUX_OFFSET)
#define CONFIG_REGISTER_MUX_AIN0_AIN3 (0x01 << CONFIG_REGISTER_MUX_OFFSET)
#define CONFIG_REGISTER_MUX_AIN1_AIN3 (0x02 << CONFIG_REGISTER_MUX_OFFSET)
#define CONFIG_REGISTER_MUX_AIN2_AIN3 (0x03 << CONFIG_REGISTER_MUX_OFFSET)
#define CONFIG_REGISTER_MUX_AIN0_GND (0x04 << CONFIG_REGISTER_MUX_OFFSET)
#define CONFIG_REGISTER_MUX_AIN1_GND (0x05 << CONFIG_REGISTER_MUX_OFFSET)
#define CONFIG_REGISTER_MUX_AIN2_GND (0x06 << CONFIG_REGISTER_MUX_OFFSET)
#define CONFIG_REGISTER_MUX_AIN3_GND (0x07 << CONFIG_REGISTER_MUX_OFFSET)

#define CONFIG_REGISTER_PGA_MASK 0x0E00
#define CONFIG_REGISTER_PGA_6144V (0x00 << CONFIG_REGISTER_PGA_OFFSET)
#define CONFIG_REGISTER_PGA_4096V (0x01 << CONFIG_REGISTER_PGA_OFFSET)
#define CONFIG_REGISTER_PGA_2048V (0x02 << CONFIG_REGISTER_PGA_OFFSET)
#define CONFIG_REGISTER_PGA_1024V (0x03 << CONFIG_REGISTER_PGA_OFFSET)
#define CONFIG_REGISTER_PGA_512V (0x04 << CONFIG_REGISTER_PGA_OFFSET)
#define CONFIG_REGISTER_PGA_256V (0x05 << CONFIG_REGISTER_PGA_OFFSET)

#define CONFIG_REGISTER_MODE_CONTINUOUS (0x00 << CONFIG_REGISTER_MODE_OFFSET)
#define CONFIG_REGISTER_MODE_SINGLE_SHOT (0x01 << CONFIG_REGISTER_MODE_OFFSET)

#define CONFIG_REGISTER_DR_128SPS (0x00 << CONFIG_REGISTER_DR_OFFSET)
#define CONFIG_REGISTER_DR_250SPS (0x01 << CONFIG_REGISTER_DR_OFFSET)
#define CONFIG_REGISTER_DR_490SPS (0x02 << CONFIG_REGISTER_DR_OFFSET)
#define CONFIG_REGISTER_DR_920SPS (0x03 << CONFIG_REGISTER_DR_OFFSET)
#define CONFIG_REGISTER_DR_1600SPS (0x04 << CONFIG_REGISTER_DR_OFFSET)
#define CONFIG_REGISTER_DR_2400SPS (0x05 << CONFIG_REGISTER_DR_OFFSET)
#define CONFIG_REGISTER_DR_3300SPS (0x06 << CONFIG_REGISTER_DR_OFFSET)

#define CONFIG_REGISTER_COMP_MODE_TRADITIONAL (0x00 << CONFIG_REGISTER_COMP_MODE_OFFSET)
#define CONFIG_REGISTER_COMP_MODE_WINDOW (0x01 << CONFIG_REGISTER_COMP_MODE_OFFSET)

#define CONFIG_REGISTER_COMP_POL_ACTIVE_LOW (0x00 << CONFIG_REGISTER_COMP_POL_OFFSET)
#define CONFIG_REGISTER_COMP_POL_ACTIVE_HIGH (0x01 << CONFIG_REGISTER_COMP_POL_OFFSET)

#define CONFIG_REGISTER_COMP_LAT_NON_LATCHING (0x00 << CONFIG_REGISTER_COMP_LAT_OFFSET)
#define CONFIG_REGISTER_COMP_LAT_LATCHING (0x01 << CONFIG_REGISTER_COMP_LAT_OFFSET)

#define CONFIG_REGISTER_COMP_QUE_ASSERT_1 (0x00 << CONFIG_REGISTER_COMP_QUE_OFFSET)
#define CONFIG_REGISTER_COMP_QUE_ASSERT_2 (0x01 << CONFIG_REGISTER_COMP_QUE_OFFSET)
#define CONFIG_REGISTER_COMP_QUE_ASSERT_4 (0x02 << CONFIG_REGISTER_COMP_QUE_OFFSET)
#define CONFIG_REGISTER_COMP_QUE_DISABLE (0x03 << CONFIG_REGISTER_COMP_QUE_OFFSET)
#define ADS1015_ADDRESS 0x49
#define ADS1015_CONVERSION_REGISTER 0x00
#define ADS1015_CONFIG_REGISTER 0x01
#define ADS1015_VOLTAGE_PER_BIT (3.3 - 0) / 4096
class ADS1015
{
public:
    ADS1015(uint8_t address = ADS1015_ADDRESS);
    void setConfig(CONFIG_REGISTER config);
    void setConfig(uint16_t config);
    uint16_t getConfig();
    int16_t readADC();
    float readVoltage(bool continuous = true);

private:
    int _file;
    uint8_t _address;
    void writeRegister(uint8_t reg, uint16_t value);
    uint16_t readRegister(uint8_t reg);
};