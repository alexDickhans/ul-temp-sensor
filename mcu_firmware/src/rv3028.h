#ifndef RV3028_H
#define RV3028_H

#include <zephyr/kernel.h>

// RV-3028 I2C address
#define RV3028_I2C_ADDR 0x52

// RV-3028 registers
#define RV3028_REG_SECONDS      0x00
#define RV3028_REG_MINUTES      0x01
#define RV3028_REG_HOURS        0x02
#define RV3028_REG_WEEKDAY      0x03
#define RV3028_REG_DATE         0x04
#define RV3028_REG_MONTH        0x05
#define RV3028_REG_YEAR         0x06
#define RV3028_REG_ALARM_SEC    0x07
#define RV3028_REG_ALARM_MIN    0x08
#define RV3028_REG_ALARM_HOUR   0x09
#define RV3028_REG_ALARM_WEEK   0x0A
#define RV3028_REG_ALARM_DATE   0x0B
#define RV3028_REG_STATUS       0x0E
#define RV3028_REG_CONTROL1     0x0F
#define RV3028_REG_CONTROL2     0x10
#define RV3028_REG_CONTROL3     0x11
#define RV3028_REG_TIMESTAMP0   0x1A
#define RV3028_REG_TIMESTAMP1   0x1B
#define RV3028_REG_TIMESTAMP2   0x1C
#define RV3028_REG_TIMESTAMP3   0x1D
#define RV3028_REG_TIMESTAMP4   0x1E
#define RV3028_REG_TIMESTAMP5   0x1F

// Control register bits
#define RV3028_CTRL1_EERD       0x80  // EEPROM read enable
#define RV3028_CTRL1_WADA       0x40  // Week/date alarm
#define RV3028_CTRL1_UTSM       0x20  // Update time stamp mode
#define RV3028_CTRL1_12_24      0x10  // 12/24 hour mode
#define RV3028_CTRL1_RESET      0x08  // Software reset
#define RV3028_CTRL1_EERE       0x04  // EEPROM read enable
#define RV3028_CTRL1_TD         0x02  // Temperature compensation disable
#define RV3028_CTRL1_CLKINT     0x01  // Clock interrupt enable

#define RV3028_CTRL2_HF         0x80  // High frequency output
#define RV3028_CTRL2_AF         0x40  // Alarm flag
#define RV3028_CTRL2_TF         0x20  // Timer flag
#define RV3028_CTRL2_UF         0x10  // Update flag
#define RV3028_CTRL2_AIE        0x08  // Alarm interrupt enable
#define RV3028_CTRL2_TIE        0x04  // Timer interrupt enable
#define RV3028_CTRL2_UIE        0x02  // Update interrupt enable
#define RV3028_CTRL2_STOP       0x01  // Stop clock

// Status register bits
#define RV3028_STATUS_VLF       0x80  // Voltage low flag
#define RV3028_STATUS_AF        0x40  // Alarm flag
#define RV3028_STATUS_TF        0x20  // Timer flag
#define RV3028_STATUS_UF        0x10  // Update flag
#define RV3028_STATUS_BSF       0x08  // Battery switch flag
#define RV3028_STATUS_CLKF      0x04  // Clock fail flag
#define RV3028_STATUS_EEBUSY    0x02  // EEPROM busy
#define RV3028_STATUS_BUSY      0x01  // Busy

// Time structure
struct rv3028_time {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t date;
    uint8_t month;
    uint16_t year;
};

// Alarm structure
struct rv3028_alarm {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t date;
};

// Function prototypes
int rv3028_init(void);
int rv3028_get_time(struct rv3028_time *time);
int rv3028_set_time(const struct rv3028_time *time);
int rv3028_set_alarm(const struct rv3028_alarm *alarm);
int rv3028_clear_alarm(void);
int rv3028_enable_alarm_interrupt(void);
int rv3028_disable_alarm_interrupt(void);
int rv3028_set_wakeup_time(uint32_t seconds_from_now);

#endif // RV3028_H
