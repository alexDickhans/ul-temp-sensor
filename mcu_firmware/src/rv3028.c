#include "rv3028.h"
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(rv3028, LOG_LEVEL_INF);

static const struct device *i2c_dev;

// Helper functions for BCD conversion
static uint8_t bcd_to_bin(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t bin_to_bcd(uint8_t bin)
{
    return ((bin / 10) << 4) | (bin % 10);
}

// I2C read/write helper functions
static int rv3028_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_write_read(i2c_dev, RV3028_I2C_ADDR, &reg, 1, data, len);
}

static int rv3028_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_write(i2c_dev, buf, 2, RV3028_I2C_ADDR);
}

static uint8_t rv3028_read_reg8(uint8_t reg)
{
    uint8_t data;
    if (rv3028_read_reg(reg, &data, 1) != 0) {
        return 0;
    }
    return data;
}

int rv3028_init(void)
{
    uint8_t status, ctrl1, ctrl2;
    
    i2c_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr,i2c));
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    // Read status register
    status = rv3028_read_reg8(RV3028_REG_STATUS);
    LOG_INF("RV3028 status: 0x%02x", status);

    // Check for voltage low flag
    if (status & RV3028_STATUS_VLF) {
        LOG_WRN("RV3028 voltage low flag set");
    }

    // Read control registers
    ctrl1 = rv3028_read_reg8(RV3028_REG_CONTROL1);
    ctrl2 = rv3028_read_reg8(RV3028_REG_CONTROL2);
    LOG_INF("RV3028 control1: 0x%02x, control2: 0x%02x", ctrl1, ctrl2);

    // Configure for 24-hour mode and enable clock interrupt
    ctrl1 |= RV3028_CTRL1_12_24 | RV3028_CTRL1_CLKINT;
    rv3028_write_reg(RV3028_REG_CONTROL1, ctrl1);

    // Clear any pending flags
    ctrl2 &= ~(RV3028_CTRL2_AF | RV3028_CTRL2_TF | RV3028_CTRL2_UF);
    rv3028_write_reg(RV3028_REG_CONTROL2, ctrl2);

    LOG_INF("RV3028 initialized successfully");
    return 0;
}

int rv3028_get_time(struct rv3028_time *time)
{
    uint8_t data[7];
    int ret;

    ret = rv3028_read_reg(RV3028_REG_SECONDS, data, 7);
    if (ret != 0) {
        LOG_ERR("Failed to read time: %d", ret);
        return ret;
    }

    time->seconds = bcd_to_bin(data[0] & 0x7F);
    time->minutes = bcd_to_bin(data[1] & 0x7F);
    time->hours = bcd_to_bin(data[2] & 0x3F);
    time->weekday = data[3] & 0x07;
    time->date = bcd_to_bin(data[4] & 0x3F);
    time->month = bcd_to_bin(data[5] & 0x1F);
    time->year = 2000 + bcd_to_bin(data[6]);

    return 0;
}

int rv3028_set_time(const struct rv3028_time *time)
{
    uint8_t data[7];
    int ret;

    data[0] = bin_to_bcd(time->seconds);
    data[1] = bin_to_bcd(time->minutes);
    data[2] = bin_to_bcd(time->hours);
    data[3] = time->weekday;
    data[4] = bin_to_bcd(time->date);
    data[5] = bin_to_bcd(time->month);
    data[6] = bin_to_bcd(time->year - 2000);

    // Stop clock before writing
    uint8_t ctrl2 = rv3028_read_reg8(RV3028_REG_CONTROL2);
    ctrl2 |= RV3028_CTRL2_STOP;
    rv3028_write_reg(RV3028_REG_CONTROL2, ctrl2);

    // Write time data
    ret = i2c_write(i2c_dev, data, 7, RV3028_I2C_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to write time: %d", ret);
        return ret;
    }

    // Start clock
    ctrl2 &= ~RV3028_CTRL2_STOP;
    rv3028_write_reg(RV3028_REG_CONTROL2, ctrl2);

    LOG_INF("Time set to: %04d-%02d-%02d %02d:%02d:%02d", 
            time->year, time->month, time->date, 
            time->hours, time->minutes, time->seconds);

    return 0;
}

int rv3028_set_alarm(const struct rv3028_alarm *alarm)
{
    uint8_t data[5];
    int ret;

    data[0] = bin_to_bcd(alarm->seconds);
    data[1] = bin_to_bcd(alarm->minutes);
    data[2] = bin_to_bcd(alarm->hours);
    data[3] = alarm->weekday;
    data[4] = bin_to_bcd(alarm->date);

    ret = i2c_write(i2c_dev, data, 5, RV3028_I2C_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to set alarm: %d", ret);
        return ret;
    }

    LOG_INF("Alarm set to: %02d:%02d:%02d", 
            alarm->hours, alarm->minutes, alarm->seconds);

    return 0;
}

int rv3028_clear_alarm(void)
{
    uint8_t ctrl2 = rv3028_read_reg8(RV3028_REG_CONTROL2);
    ctrl2 &= ~RV3028_CTRL2_AF;
    return rv3028_write_reg(RV3028_REG_CONTROL2, ctrl2);
}

int rv3028_enable_alarm_interrupt(void)
{
    uint8_t ctrl2 = rv3028_read_reg8(RV3028_REG_CONTROL2);
    ctrl2 |= RV3028_CTRL2_AIE;
    return rv3028_write_reg(RV3028_REG_CONTROL2, ctrl2);
}

int rv3028_disable_alarm_interrupt(void)
{
    uint8_t ctrl2 = rv3028_read_reg8(RV3028_REG_CONTROL2);
    ctrl2 &= ~RV3028_CTRL2_AIE;
    return rv3028_write_reg(RV3028_REG_CONTROL2, ctrl2);
}

int rv3028_set_wakeup_time(uint32_t seconds_from_now)
{
    struct rv3028_time current_time;
    struct rv3028_alarm alarm;
    uint32_t target_seconds;
    int ret;

    // Get current time
    ret = rv3028_get_time(&current_time);
    if (ret != 0) {
        return ret;
    }

    // Calculate target time
    target_seconds = (current_time.hours * 3600) + 
                    (current_time.minutes * 60) + 
                    current_time.seconds + 
                    seconds_from_now;

    // Convert back to time components
    alarm.hours = (target_seconds / 3600) % 24;
    alarm.minutes = (target_seconds / 60) % 60;
    alarm.seconds = target_seconds % 60;
    alarm.weekday = current_time.weekday;
    alarm.date = current_time.date;

    // Set alarm
    ret = rv3028_set_alarm(&alarm);
    if (ret != 0) {
        return ret;
    }

    // Enable alarm interrupt
    return rv3028_enable_alarm_interrupt();
}
