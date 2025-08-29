#ifndef ADAPTIVE_SCHEDULER_H
#define ADAPTIVE_SCHEDULER_H

#include <zephyr/kernel.h>

// Power tiers based on battery voltage
typedef enum {
    POWER_TIER_NORMAL = 0,    // 4.2V - 3.8V: 5 min intervals
    POWER_TIER_CONSERVE = 1,  // 3.8V - 3.6V: 15 min intervals  
    POWER_TIER_RESERVE = 2,   // 3.6V - 3.4V: 30 min intervals
    POWER_TIER_SURVIVAL = 3   // 3.4V - 3.2V: 60 min intervals
} power_tier_t;

// Wake intervals for each tier (in milliseconds)
#define WAKE_INTERVAL_NORMAL    (5 * 60 * 1000)   // 5 minutes
#define WAKE_INTERVAL_CONSERVE  (15 * 60 * 1000)  // 15 minutes
#define WAKE_INTERVAL_RESERVE   (30 * 60 * 1000)  // 30 minutes
#define WAKE_INTERVAL_SURVIVAL  (60 * 60 * 1000)  // 60 minutes

// Battery voltage thresholds with hysteresis (in mV)
#define BATTERY_THRESHOLD_NORMAL_HIGH    3800
#define BATTERY_THRESHOLD_NORMAL_LOW     3600
#define BATTERY_THRESHOLD_CONSERVE_HIGH  3600
#define BATTERY_THRESHOLD_CONSERVE_LOW   3400
#define BATTERY_THRESHOLD_RESERVE_HIGH   3400
#define BATTERY_THRESHOLD_RESERVE_LOW    3200

// Function prototypes
int adaptive_scheduler_init(void);
power_tier_t adaptive_scheduler_get_tier(uint16_t battery_mv);
uint32_t adaptive_scheduler_get_interval(power_tier_t tier);
int adaptive_scheduler_set_next_wake(uint32_t interval_ms);

#endif // ADAPTIVE_SCHEDULER_H
