#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include "bme280.h"
#include "battery_monitor.h"
#include "rv3028.h"
#include "adaptive_scheduler.h"
#include "ble_advertiser.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// Power management callback
static void pm_state_set(enum pm_state state, uint8_t substate_id)
{
    ARG_UNUSED(substate_id);
    LOG_INF("Entering power state %d", state);
}

static void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
    ARG_UNUSED(state);
    ARG_UNUSED(substate_id);
    LOG_INF("Exiting power state");
}

// Power management callbacks
PM_STATE_SET_AND_EXIT_POST_OPS(pm_state_set, pm_state_exit_post_ops);

// Main application thread
static void main_thread(void)
{
    int ret;
    struct bme280_data sensor_data;
    uint16_t battery_mv;
    power_tier_t current_tier;
    uint32_t next_wake_interval;

    LOG_INF("Temperature Sensor Node Starting...");

    // Initialize subsystems
    ret = bme280_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize BME280: %d", ret);
        return;
    }

    ret = battery_monitor_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize battery monitor: %d", ret);
        return;
    }

    ret = adaptive_scheduler_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize adaptive scheduler: %d", ret);
        return;
    }

    ret = ble_advertiser_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize BLE advertiser: %d", ret);
        return;
    }

    LOG_INF("All subsystems initialized successfully");

    while (1) {
        // Read battery voltage and determine power tier
        battery_mv = battery_monitor_read_voltage();
        current_tier = adaptive_scheduler_get_tier(battery_mv);
        next_wake_interval = adaptive_scheduler_get_interval(current_tier);

        LOG_INF("Battery: %d mV, Tier: %d, Next wake: %lu ms", 
                battery_mv, current_tier, next_wake_interval);

        // Read sensor data
        ret = bme280_read_forced(&sensor_data);
        if (ret == 0) {
            LOG_INF("Sensor: T=%.2f°C, P=%.2f hPa, H=%.2f%%", 
                    sensor_data.temperature, sensor_data.pressure, sensor_data.humidity);
        } else {
            LOG_ERR("Failed to read sensor: %d", ret);
            // Use last known values or defaults
            sensor_data.temperature = 0.0f;
            sensor_data.pressure = 1013.25f;
            sensor_data.humidity = 50.0f;
        }

        // Start BLE advertising with sensor data
        ret = ble_advertiser_start(&sensor_data, battery_mv, current_tier);
        if (ret != 0) {
            LOG_ERR("Failed to start advertising: %d", ret);
        }

        // Wait for advertising to complete
        k_sleep(K_MSEC(ADV_DURATION_MS));

        // Stop advertising
        ble_advertiser_stop();

        // Set RTC alarm for next wake
        ret = adaptive_scheduler_set_next_wake(next_wake_interval);
        if (ret != 0) {
            LOG_ERR("Failed to set RTC alarm: %d", ret);
        }

        LOG_INF("Entering deep sleep for %lu ms", next_wake_interval);

        // Enter system OFF mode
        pm_state_force(0u, PM_STATE_SOFT_OFF);
        
        // This should not be reached - system will wake from RTC
        k_sleep(K_MSEC(100));
    }
}

K_THREAD_DEFINE(main_thread_id, 2048, main_thread, NULL, NULL, NULL, 7, 0, 0);
