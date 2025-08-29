#include "adaptive_scheduler.h"
#include "rv3028.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adaptive_scheduler, LOG_LEVEL_INF);

static const struct device *rtc_int_gpio;
static power_tier_t current_tier = POWER_TIER_NORMAL;

int adaptive_scheduler_init(void)
{
    // Initialize RV-3028 RTC
    int ret = rv3028_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize RV-3028: %d", ret);
        return ret;
    }

    // Configure RTC interrupt GPIO (P0.02)
    rtc_int_gpio = DEVICE_DT_GET(DT_ALIAS(rtc_int));
    if (!device_is_ready(rtc_int_gpio)) {
        LOG_ERR("RTC interrupt GPIO not ready");
        return -ENODEV;
    }

    // Configure as input with pull-up
    gpio_flags_t flags = GPIO_INPUT | GPIO_PULL_UP;
    ret = gpio_pin_configure(rtc_int_gpio, 2, flags);
    if (ret != 0) {
        LOG_ERR("Failed to configure RTC interrupt GPIO: %d", ret);
        return ret;
    }

    LOG_INF("Adaptive scheduler initialized with RV-3028");
    return 0;
}

power_tier_t adaptive_scheduler_get_tier(uint16_t battery_mv)
{
    power_tier_t new_tier;
    
    // Determine tier based on battery voltage with hysteresis
    if (battery_mv >= BATTERY_THRESHOLD_NORMAL_HIGH) {
        new_tier = POWER_TIER_NORMAL;
    } else if (battery_mv >= BATTERY_THRESHOLD_CONSERVE_HIGH) {
        new_tier = POWER_TIER_CONSERVE;
    } else if (battery_mv >= BATTERY_THRESHOLD_RESERVE_HIGH) {
        new_tier = POWER_TIER_RESERVE;
    } else {
        new_tier = POWER_TIER_SURVIVAL;
    }
    
    // Apply hysteresis to prevent tier flapping
    if (new_tier != current_tier) {
        // Only allow tier changes in one direction at a time
        if (new_tier > current_tier) {
            // Moving to higher power tier (lower battery) - use low threshold
            if (battery_mv <= BATTERY_THRESHOLD_NORMAL_LOW && current_tier == POWER_TIER_NORMAL) {
                new_tier = POWER_TIER_CONSERVE;
            } else if (battery_mv <= BATTERY_THRESHOLD_CONSERVE_LOW && current_tier == POWER_TIER_CONSERVE) {
                new_tier = POWER_TIER_RESERVE;
            } else if (battery_mv <= BATTERY_THRESHOLD_RESERVE_LOW && current_tier == POWER_TIER_RESERVE) {
                new_tier = POWER_TIER_SURVIVAL;
            } else {
                new_tier = current_tier; // Stay in current tier
            }
        } else {
            // Moving to lower power tier (higher battery) - use high threshold
            if (battery_mv >= BATTERY_THRESHOLD_NORMAL_HIGH && current_tier == POWER_TIER_CONSERVE) {
                new_tier = POWER_TIER_NORMAL;
            } else if (battery_mv >= BATTERY_THRESHOLD_CONSERVE_HIGH && current_tier == POWER_TIER_RESERVE) {
                new_tier = POWER_TIER_CONSERVE;
            } else if (battery_mv >= BATTERY_THRESHOLD_RESERVE_HIGH && current_tier == POWER_TIER_SURVIVAL) {
                new_tier = POWER_TIER_RESERVE;
            } else {
                new_tier = current_tier; // Stay in current tier
            }
        }
    }
    
    if (new_tier != current_tier) {
        LOG_INF("Power tier changed: %d -> %d (battery: %d mV)", current_tier, new_tier, battery_mv);
        current_tier = new_tier;
    }
    
    return current_tier;
}

uint32_t adaptive_scheduler_get_interval(power_tier_t tier)
{
    switch (tier) {
        case POWER_TIER_NORMAL:
            return WAKE_INTERVAL_NORMAL;
        case POWER_TIER_CONSERVE:
            return WAKE_INTERVAL_CONSERVE;
        case POWER_TIER_RESERVE:
            return WAKE_INTERVAL_RESERVE;
        case POWER_TIER_SURVIVAL:
            return WAKE_INTERVAL_SURVIVAL;
        default:
            return WAKE_INTERVAL_NORMAL;
    }
}

int adaptive_scheduler_set_next_wake(uint32_t interval_ms)
{
    int ret;
    
    // Clear any existing alarm
    rv3028_clear_alarm();
    
    // Set wakeup time using RV-3028
    ret = rv3028_set_wakeup_time(interval_ms / 1000);
    if (ret != 0) {
        LOG_ERR("Failed to set RV-3028 wakeup time: %d", ret);
        return ret;
    }
    
    LOG_INF("RV-3028 alarm set for %lu seconds from now", interval_ms / 1000);
    return 0;
}
