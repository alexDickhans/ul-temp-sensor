#ifndef BLE_ADVERTISER_H
#define BLE_ADVERTISER_H

#include <zephyr/kernel.h>
#include "bme280.h"
#include "adaptive_scheduler.h"

// BLE advertising configuration
#define ADV_DURATION_MS        30000  // 30 seconds advertising duration
#define ADV_INTERVAL_NORMAL    1000   // 1 Hz for normal tier
#define ADV_INTERVAL_CONSERVE  5000   // 0.2 Hz for conserve tier
#define ADV_INTERVAL_RESERVE   10000  // 0.1 Hz for reserve tier

// Manufacturer data structure (custom payload)
struct sensor_adv_data {
    uint8_t version;           // Protocol version (1)
    uint8_t tier;              // Power tier
    uint16_t battery_mv;       // Battery voltage in mV
    int16_t temperature;       // Temperature * 100 (to preserve 2 decimal places)
    uint16_t pressure;         // Pressure * 10 (to preserve 1 decimal place)
    uint16_t humidity;         // Humidity * 100 (to preserve 2 decimal places)
    uint32_t timestamp;        // Unix timestamp (if available)
} __attribute__((packed));

// Function prototypes
int ble_advertiser_init(void);
int ble_advertiser_start(const struct bme280_data *sensor_data, uint16_t battery_mv, power_tier_t tier);
int ble_advertiser_stop(void);

#endif // BLE_ADVERTISER_H
