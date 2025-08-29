#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <zephyr/kernel.h>

// Battery voltage thresholds (in mV)
#define BATTERY_VOLTAGE_MAX     4200  // 4.2V fully charged
#define BATTERY_VOLTAGE_MIN     3500  // 3.5V minimum safe voltage
#define BATTERY_VOLTAGE_CRITICAL 3300 // 3.3V critical low

// ADC configuration
#define ADC_RESOLUTION          12    // 12-bit ADC
#define ADC_REFERENCE_VOLTAGE   3300  // 3.3V reference
#define ADC_GAIN                3.256 // Voltage divider ratio: (4.22M + 1.87M) / 1.87M ≈ 3.256

// Function prototypes
int battery_monitor_init(void);
uint16_t battery_monitor_read_voltage(void);
uint8_t battery_monitor_get_percentage(uint16_t voltage_mv);

#endif // BATTERY_MONITOR_H
