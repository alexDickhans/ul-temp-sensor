#include "ble_advertiser.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(ble_advertiser, LOG_LEVEL_INF);

// Nordic Semiconductor Company ID
#define NORDIC_COMPANY_ID 0x0059

// Advertising data buffer
static uint8_t adv_data[31];
static uint8_t adv_data_len;

// Get advertising interval based on power tier
static uint16_t get_adv_interval(power_tier_t tier)
{
    switch (tier) {
        case POWER_TIER_NORMAL:
            return ADV_INTERVAL_NORMAL;
        case POWER_TIER_CONSERVE:
            return ADV_INTERVAL_CONSERVE;
        case POWER_TIER_RESERVE:
        case POWER_TIER_SURVIVAL:
            return ADV_INTERVAL_RESERVE;
        default:
            return ADV_INTERVAL_NORMAL;
    }
}

// Prepare advertising data with sensor information
static int prepare_adv_data(const struct bme280_data *sensor_data, 
                           uint16_t battery_mv, power_tier_t tier)
{
    struct sensor_adv_data sensor_payload;
    uint8_t *ptr = adv_data;
    uint8_t remaining = sizeof(adv_data);
    
    // Clear buffer
    memset(adv_data, 0, sizeof(adv_data));
    
    // Flags (LE General Discoverable Mode)
    if (remaining < 3) return -ENOMEM;
    *ptr++ = 2;  // Length
    *ptr++ = BT_DATA_FLAGS;
    *ptr++ = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;
    remaining -= 3;
    
    // Complete Local Name
    const char *name = "TempSensor";
    uint8_t name_len = strlen(name);
    if (remaining < name_len + 2) return -ENOMEM;
    *ptr++ = name_len + 1;  // Length
    *ptr++ = BT_DATA_NAME_COMPLETE;
    memcpy(ptr, name, name_len);
    ptr += name_len;
    remaining -= (name_len + 2);
    
    // Manufacturer Specific Data
    sensor_payload.version = 1;
    sensor_payload.tier = (uint8_t)tier;
    sensor_payload.battery_mv = battery_mv;
    sensor_payload.temperature = (int16_t)(sensor_data->temperature * 100);
    sensor_payload.pressure = (uint16_t)(sensor_data->pressure * 10);
    sensor_payload.humidity = (uint16_t)(sensor_data->humidity * 100);
    sensor_payload.timestamp = k_uptime_get() / 1000; // Current uptime in seconds
    
    uint8_t mfg_data_len = sizeof(sensor_payload) + 4; // +4 for company ID
    if (remaining < mfg_data_len + 1) return -ENOMEM;
    
    *ptr++ = mfg_data_len;  // Length
    *ptr++ = BT_DATA_MANUFACTURER_DATA;
    
    // Company ID (little endian)
    *ptr++ = NORDIC_COMPANY_ID & 0xFF;
    *ptr++ = (NORDIC_COMPANY_ID >> 8) & 0xFF;
    
    // Sensor payload
    memcpy(ptr, &sensor_payload, sizeof(sensor_payload));
    ptr += sizeof(sensor_payload);
    
    adv_data_len = ptr - adv_data;
    
    LOG_DBG("Advertising data prepared: %d bytes", adv_data_len);
    LOG_DBG("Payload: T=%.2f°C, P=%.1f hPa, H=%.2f%%, V=%d mV, Tier=%d",
            sensor_data->temperature, sensor_data->pressure, 
            sensor_data->humidity, battery_mv, tier);
    
    return 0;
}

int ble_advertiser_init(void)
{
    int ret;
    
    // Initialize Bluetooth
    ret = bt_enable(NULL);
    if (ret != 0) {
        LOG_ERR("Failed to enable Bluetooth: %d", ret);
        return ret;
    }
    
    LOG_INF("Bluetooth initialized successfully");
    return 0;
}

int ble_advertiser_start(const struct bme280_data *sensor_data, 
                        uint16_t battery_mv, power_tier_t tier)
{
    int ret;
    struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = BT_LE_ADV_OPT_NONE,
        .interval_min = get_adv_interval(tier),
        .interval_max = get_adv_interval(tier),
        .peer = NULL,
    };
    
    // Prepare advertising data
    ret = prepare_adv_data(sensor_data, battery_mv, tier);
    if (ret != 0) {
        LOG_ERR("Failed to prepare advertising data: %d", ret);
        return ret;
    }
    
    // Start advertising (non-connectable)
    ret = bt_le_adv_start(&adv_param, adv_data, adv_data_len, NULL, 0);
    if (ret != 0) {
        LOG_ERR("Failed to start advertising: %d", ret);
        return ret;
    }
    
    LOG_INF("BLE advertising started (tier %d, interval %d ms)", 
            tier, get_adv_interval(tier));
    return 0;
}

int ble_advertiser_stop(void)
{
    int ret = bt_le_adv_stop();
    if (ret != 0) {
        LOG_ERR("Failed to stop advertising: %d", ret);
        return ret;
    }
    
    LOG_INF("BLE advertising stopped");
    return 0;
}
