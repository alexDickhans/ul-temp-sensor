#ifndef BME280_H
#define BME280_H

#include <zephyr/kernel.h>

// BME280 I2C address
#define BME280_I2C_ADDR 0x76

// BME280 registers
#define BME280_REG_TEMP_MSB    0xFA
#define BME280_REG_TEMP_LSB    0xFB
#define BME280_REG_TEMP_XLSB   0xFC
#define BME280_REG_PRESS_MSB   0xF7
#define BME280_REG_PRESS_LSB   0xF8
#define BME280_REG_PRESS_XLSB  0xF9
#define BME280_REG_HUM_MSB     0xFD
#define BME280_REG_HUM_LSB     0xFE
#define BME280_REG_CONFIG      0xF5
#define BME280_REG_CTRL_MEAS   0xF4
#define BME280_REG_CTRL_HUM    0xF2
#define BME280_REG_CHIP_ID     0xD0
#define BME280_REG_RESET       0xE0

// Calibration registers
#define BME280_REG_DIG_T1      0x88
#define BME280_REG_DIG_T2      0x8A
#define BME280_REG_DIG_T3      0x8C
#define BME280_REG_DIG_P1      0x8E
#define BME280_REG_DIG_P2      0x90
#define BME280_REG_DIG_P3      0x92
#define BME280_REG_DIG_P4      0x94
#define BME280_REG_DIG_P5      0x96
#define BME280_REG_DIG_P6      0x98
#define BME280_REG_DIG_P7      0x9A
#define BME280_REG_DIG_P8      0x9C
#define BME280_REG_DIG_P9      0x9E
#define BME280_REG_DIG_H1      0xA1
#define BME280_REG_DIG_H2      0xE1
#define BME280_REG_DIG_H3      0xE3
#define BME280_REG_DIG_H4      0xE4
#define BME280_REG_DIG_H5      0xE5
#define BME280_REG_DIG_H6      0xE7

// Control register values
#define BME280_CTRL_HUM_OSRS_H_1X    0x01
#define BME280_CTRL_MEAS_OSRS_T_1X   0x20
#define BME280_CTRL_MEAS_OSRS_P_1X   0x04
#define BME280_CTRL_MEAS_MODE_FORCED 0x01

// Sensor data structure
struct bme280_data {
    float temperature;  // Celsius
    float pressure;     // hPa
    float humidity;     // %
};

// Calibration data structure
struct bme280_calib_data {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
};

// Function prototypes
int bme280_init(void);
int bme280_read_forced(struct bme280_data *data);
int bme280_read_calibration_data(void);

#endif // BME280_H
