#include "battery_monitor.h"
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(battery_monitor, LOG_LEVEL_INF);

static const struct device *adc_dev;
static const struct adc_channel_cfg channel_cfg = {
    .gain = ADC_GAIN_1_4,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = 0,
    .differential = 0,
    .input_positive = 2,  // P0.02 for VBAT_SENSE
};

int battery_monitor_init(void)
{
    adc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr,adc));
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device not ready");
        return -ENODEV;
    }

    // Configure ADC channel
    if (adc_channel_setup(adc_dev, &channel_cfg) != 0) {
        LOG_ERR("Failed to setup ADC channel");
        return -EIO;
    }

    LOG_INF("Battery monitor initialized");
    return 0;
}

uint16_t battery_monitor_read_voltage(void)
{
    int ret;
    uint16_t adc_value;
    uint16_t voltage_mv;
    struct adc_sequence sequence = {
        .buffer = &adc_value,
        .buffer_size = sizeof(adc_value),
        .resolution = ADC_RESOLUTION,
        .oversampling = 4, // Average 4 samples for better accuracy
    };

    // Read ADC value
    ret = adc_read(adc_dev, &sequence);
    if (ret != 0) {
        LOG_ERR("Failed to read ADC: %d", ret);
        return 0;
    }

    // Convert ADC value to voltage (mV)
    // Assuming voltage divider: V_bat / 2 = V_adc
    // V_bat = V_adc * 2 * ADC_REFERENCE_VOLTAGE / (2^ADC_RESOLUTION)
    voltage_mv = (uint16_t)((uint32_t)adc_value * ADC_REFERENCE_VOLTAGE * ADC_GAIN / (1 << ADC_RESOLUTION));

    LOG_DBG("ADC: %d, Voltage: %d mV", adc_value, voltage_mv);
    return voltage_mv;
}

uint8_t battery_monitor_get_percentage(uint16_t voltage_mv)
{
    uint8_t percentage;
    
    if (voltage_mv >= BATTERY_VOLTAGE_MAX) {
        percentage = 100;
    } else if (voltage_mv <= BATTERY_VOLTAGE_MIN) {
        percentage = 0;
    } else {
        // Linear interpolation between min and max voltage
        percentage = (uint8_t)((voltage_mv - BATTERY_VOLTAGE_MIN) * 100 / 
                              (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN));
    }
    
    return percentage;
}
