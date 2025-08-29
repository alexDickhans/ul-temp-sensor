#include "bme280.h"
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(bme280, LOG_LEVEL_INF);

static const struct device *i2c_dev;
static struct bme280_calib_data calib_data;

// I2C read/write helper functions
static int bme280_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_write_read(i2c_dev, BME280_I2C_ADDR, &reg, 1, data, len);
}

static int bme280_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_write(i2c_dev, buf, 2, BME280_I2C_ADDR);
}

static uint8_t bme280_read_reg8(uint8_t reg)
{
    uint8_t data;
    if (bme280_read_reg(reg, &data, 1) != 0) {
        return 0;
    }
    return data;
}

static uint16_t bme280_read_reg16(uint8_t reg)
{
    uint8_t data[2];
    if (bme280_read_reg(reg, data, 2) != 0) {
        return 0;
    }
    return (data[0] << 8) | data[1];
}

static int16_t bme280_read_reg16_signed(uint8_t reg)
{
    return (int16_t)bme280_read_reg16(reg);
}

int bme280_init(void)
{
    uint8_t chip_id;
    
    i2c_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr,i2c));
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    // Check chip ID
    chip_id = bme280_read_reg8(BME280_REG_CHIP_ID);
    if (chip_id != 0x60) {
        LOG_ERR("Invalid chip ID: 0x%02x", chip_id);
        return -ENODEV;
    }

    LOG_INF("BME280 found, chip ID: 0x%02x", chip_id);

    // Read calibration data
    if (bme280_read_calibration_data() != 0) {
        LOG_ERR("Failed to read calibration data");
        return -EIO;
    }

    // Configure humidity control register
    if (bme280_write_reg(BME280_REG_CTRL_HUM, BME280_CTRL_HUM_OSRS_H_1X) != 0) {
        LOG_ERR("Failed to configure humidity control");
        return -EIO;
    }

    // Configure measurement control register for forced mode
    uint8_t ctrl_meas = BME280_CTRL_MEAS_OSRS_T_1X | 
                       BME280_CTRL_MEAS_OSRS_P_1X | 
                       BME280_CTRL_MEAS_MODE_FORCED;
    
    if (bme280_write_reg(BME280_REG_CTRL_MEAS, ctrl_meas) != 0) {
        LOG_ERR("Failed to configure measurement control");
        return -EIO;
    }

    LOG_INF("BME280 initialized successfully");
    return 0;
}

int bme280_read_calibration_data(void)
{
    // Temperature calibration
    calib_data.dig_T1 = bme280_read_reg16(BME280_REG_DIG_T1);
    calib_data.dig_T2 = bme280_read_reg16_signed(BME280_REG_DIG_T2);
    calib_data.dig_T3 = bme280_read_reg16_signed(BME280_REG_DIG_T3);

    // Pressure calibration
    calib_data.dig_P1 = bme280_read_reg16(BME280_REG_DIG_P1);
    calib_data.dig_P2 = bme280_read_reg16_signed(BME280_REG_DIG_P2);
    calib_data.dig_P3 = bme280_read_reg16_signed(BME280_REG_DIG_P3);
    calib_data.dig_P4 = bme280_read_reg16_signed(BME280_REG_DIG_P4);
    calib_data.dig_P5 = bme280_read_reg16_signed(BME280_REG_DIG_P5);
    calib_data.dig_P6 = bme280_read_reg16_signed(BME280_REG_DIG_P6);
    calib_data.dig_P7 = bme280_read_reg16_signed(BME280_REG_DIG_P7);
    calib_data.dig_P8 = bme280_read_reg16_signed(BME280_REG_DIG_P8);
    calib_data.dig_P9 = bme280_read_reg16_signed(BME280_REG_DIG_P9);

    // Humidity calibration
    calib_data.dig_H1 = bme280_read_reg8(BME280_REG_DIG_H1);
    calib_data.dig_H2 = bme280_read_reg16_signed(BME280_REG_DIG_H2);
    calib_data.dig_H3 = bme280_read_reg8(BME280_REG_DIG_H3);
    
    uint8_t h4_msb = bme280_read_reg8(BME280_REG_DIG_H4);
    uint8_t h4_lsb = bme280_read_reg8(BME280_REG_DIG_H4 + 1);
    calib_data.dig_H4 = (h4_msb << 4) | (h4_lsb & 0x0F);
    
    uint8_t h5_msb = bme280_read_reg8(BME280_REG_DIG_H5 + 1);
    uint8_t h5_lsb = bme280_read_reg8(BME280_REG_DIG_H5);
    calib_data.dig_H5 = (h5_msb << 4) | ((h5_lsb >> 4) & 0x0F);
    
    calib_data.dig_H6 = (int8_t)bme280_read_reg8(BME280_REG_DIG_H6);

    LOG_INF("Calibration data loaded");
    return 0;
}

int bme280_read_forced(struct bme280_data *data)
{
    uint8_t raw_data[8];
    int32_t adc_T, adc_P, adc_H;
    int32_t var1, var2, t_fine;
    int32_t p, h;

    // Trigger forced measurement
    uint8_t ctrl_meas = BME280_CTRL_MEAS_OSRS_T_1X | 
                       BME280_CTRL_MEAS_OSRS_P_1X | 
                       BME280_CTRL_MEAS_MODE_FORCED;
    
    if (bme280_write_reg(BME280_REG_CTRL_MEAS, ctrl_meas) != 0) {
        LOG_ERR("Failed to trigger measurement");
        return -EIO;
    }

    // Wait for measurement to complete (max 10ms)
    k_sleep(K_MSEC(10));

    // Read all sensor data
    if (bme280_read_reg(BME280_REG_PRESS_MSB, raw_data, 8) != 0) {
        LOG_ERR("Failed to read sensor data");
        return -EIO;
    }

    // Extract raw ADC values
    adc_P = ((int32_t)raw_data[0] << 12) | ((int32_t)raw_data[1] << 4) | (raw_data[2] >> 4);
    adc_T = ((int32_t)raw_data[3] << 12) | ((int32_t)raw_data[4] << 4) | (raw_data[5] >> 4);
    adc_H = ((int32_t)raw_data[6] << 8) | raw_data[7];

    // Temperature compensation (returns temperature in 0.01°C)
    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * 
            ((int32_t)calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * 
              ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * 
            ((int32_t)calib_data.dig_T3)) >> 14;
    t_fine = var1 + var2;
    data->temperature = (float)((t_fine * 5 + 128) >> 8) / 100.0f;

    // Pressure compensation
    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)calib_data.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)calib_data.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)calib_data.dig_P4) << 16);
    var1 = (((calib_data.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + 
            ((((int32_t)calib_data.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)calib_data.dig_P1)) >> 15);
    if (var1 == 0) {
        return -EIO;
    }
    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000) {
        p = (p << 1) / ((uint32_t)var1);
    } else {
        p = (p / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)calib_data.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)calib_data.dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + calib_data.dig_P7) >> 4));
    data->pressure = (float)p / 256.0f;

    // Humidity compensation
    var1 = (t_fine - ((int32_t)76800));
    var2 = ((((adc_H << 14) - (((int32_t)calib_data.dig_H4) << 20) - 
              (((int32_t)calib_data.dig_H5) * var1)) + (int32_t)16384) >> 15) * 
           (((((((var1 * ((int32_t)calib_data.dig_H6)) >> 10) * 
                (((var1 * ((int32_t)calib_data.dig_H3)) >> 11) + (int32_t)32768)) >> 10) + 
              (int32_t)2097152) * ((int32_t)calib_data.dig_H2) + 8192) >> 14);
    var1 = var1 - ((var2 * (((var1 * ((int32_t)calib_data.dig_H6)) >> 10) * 
                            (((var1 * ((int32_t)calib_data.dig_H3)) >> 11) + (int32_t)32768))) >> 10);
    var1 = (var1 < 0 ? 0 : var1);
    var1 = (var1 > 419430400 ? 419430400 : var1);
    h = (uint32_t)(var1 >> 12);
    data->humidity = (float)h / 1024.0f;

    return 0;
}
