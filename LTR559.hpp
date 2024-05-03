#include <cstdint>
/* LTR-559 Registers */
#define LTR559_ALS_CONTROL 0x80
#define LTR559_ALS_CONTROL_GAIN_MASK 0b111
#define LTR559_ALS_CONTROL_GAIN_SHIFT 2
#define LTR559_ALS_CONTROL_SW_RESET_BIT 1
#define LTR559_ALS_CONTROL_MODE_BIT 0

#define LTR559_PS_CONTROL 0x81
#define LTR559_PS_CONTROL_SATURATION_INDICATOR_ENABLE_BIT 5
#define LTR559_PS_CONTROL_ACTIVE_MASK 0b11

#define LTR559_PS_LED 0x82
#define LTR559_PS_LED_PULSE_FREQ_MASK 0b111
#define LTR559_PS_LED_PULSE_FREQ_SHIFT 5
#define LTR559_PS_LED_DUTY_CYCLE_MASK 0b11
#define LTR559_PS_LED_DUTY_CYCLE_SHIFT 3
#define LTR559_PS_LED_CURRENT_MASK 0b111

#define LTR559_PS_N_PULSES 0x83
#define LTR559_PS_N_PULSES_MASK 0b1111

#define LTR559_PS_MEAS_RATE 0x84
#define LTR559_PS_MEAS_RATE_RATE_MS_MASK 0b1111

#define LTR559_ALS_MEAS_RATE 0x85
#define LTR559_ALS_MEAS_RATE_INTEGRATION_TIME_MASK 0b111
#define LTR559_ALS_MEAS_RATE_INTEGRATION_TIME_SHIFT 3
#define LTR559_ALS_MEAS_RATE_REPEAT_RATE_MASK 0b111

#define LTR559_PART_ID 0x86
#define LTR559_PART_ID_PART_NUMBER_MASK 0b1111
#define LTR559_PART_ID_PART_NUMBER_SHIFT 4
#define LTR559_PART_ID_REVISION_MASK 0b1111

#define LTR559_MANUFACTURER_ID 0x87

#define LTR559_ALS_DATA 0x88
#define LTR559_ALS_DATA_CH1 0x88
#define LTR559_ALS_DATA_CH0 0x8a

#define LTR559_ALS_PS_STATUS 0x8c
#define LTR559_ALS_PS_STATUS_INTERRUPT_MASK 0b00001010
#define LTR559_ALS_PS_STATUS_ALS_DATA_VALID_BIT 7
#define LTR559_ALS_PS_STATUS_ALS_GAIN_MASK 0b111
#define LTR559_ALS_PS_STATUS_ALS_GAIN_SHIFT 4
#define LTR559_ALS_PS_STATUS_ALS_INTERRUPT_BIT 3
#define LTR559_ALS_PS_STATUS_ALS_DATA_BIT 2
#define LTR559_ALS_PS_STATUS_PS_INTERRUPT_BIT 1
#define LTR559_ALS_PS_STATUS_PS_DATA_BIT 0

#define LTR559_PS_DATA 0x8d
#define LTR559_PS_DATA_MASK 0x07FF

#define LTR559_PS_DATA_SATURATION 0x8e
#define LTR559_PS_DATA_SATURATION_SHIFT 4

#define LTR559_INTERRUPT 0x8f
#define LTR559_INTERRUPT_POLARITY_BIT 2
#define LTR559_INTERRUPT_ALS_PS_MASK 0b11
#define LTR559_INTERRUPT_PS_BIT 0
#define LTR559_INTERRUPT_ALS_BIT 1

#define LTR559_PS_THRESHOLD_UPPER 0x90
#define LTR559_PS_THRESHOLD_LOWER 0x92

#define LTR559_PS_OFFSET 0x94
#define LTR559_PS_OFFSET_MASK 0x03FF

#define LTR559_ALS_THRESHOLD_UPPER 0x97
#define LTR559_ALS_THRESHOLD_LOWER 0x99

#define LTR559_INTERRUPT_PERSIST 0x9e
#define LTR559_INTERRUPT_PERSIST_PS_MASK 0b1111
#define LTR559_INTERRUPT_PERSIST_PS_SHIFT 4
#define LTR559_INTERRUPT_PERSIST_ALS_MASK 0b1111

#define LTR559_VALID_PART_ID 0x09
#define LTR559_VALID_REVISION_ID 0x02

class LTR559 {
public:
    LTR559();

    int32_t getLux();
    uint8_t getProximity();

private:
    int file;
    float _lux;

    void writeRegister(uint8_t reg, uint8_t data);
    uint8_t readRegister(uint8_t reg);
    int16_t readRegisterInt16(uint8_t offset);
};