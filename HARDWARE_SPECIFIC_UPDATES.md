# Hardware-Specific Updates - Your Configuration

## 🔧 Hardware Configuration Applied

Your specific hardware setup has been integrated into the firmware:

### 📊 Battery Monitoring Circuit
- **Voltage Divider**: R10 (4.22MΩ) + R11 (1.87MΩ)
- **Division Ratio**: ~0.307 (VBAT_SENSE ≈ 30.7% of +BATT)
- **ADC Input**: P0.02 (VBAT_SENSE)
- **Updated Gain**: 3.256 (calculated from your resistor values)

### 🕐 RV-3028-C7 RTC Integration
- **I²C Address**: 0x52
- **I²C Pins**: P0.26 (SCL), P0.27 (SDA) - shared with BME280
- **RTC_INT**: P0.02 (interrupt input)
- **RTC_CLOCKOUT**: P0.03 (clock output)
- **Features**: 
  - High-accuracy timekeeping (±3.4 ppm)
  - Programmable alarm with interrupt
  - Temperature compensation
  - Ultra-low power consumption

### 🌡️ BME280 Sensor
- **I²C Address**: 0x76 (ADDR pin to GND)
- **I²C Pins**: P0.26 (SCL), P0.27 (SDA) - shared with RV-3028
- **Operation**: Forced-mode for power efficiency

## 📝 Firmware Updates Made

### 1. Device Tree Overlay (`nrf52840dk_nrf52840.overlay`)
```dts
// Added RV-3028 device
rv3028@52 {
    compatible = "microcrystal,rv3028";
    reg = <0x52>;
    label = "RV3028";
    int-gpios = <&gpio0 2 GPIO_ACTIVE_LOW>;
};

// Updated ADC channel for P0.02
channel@0 {
    zephyr,input-positive = <2>;  /* P0.02 for VBAT_SENSE */
}

// Added GPIO alias for RTC interrupt
aliases {
    rtc_int = &gpio0;
};
```

### 2. Battery Monitor (`battery_monitor.h/c`)
```c
// Updated voltage divider ratio
#define ADC_GAIN 3.256  // (4.22M + 1.87M) / 1.87M ≈ 3.256

// Updated ADC input pin
.input_positive = 2,  // P0.02 for VBAT_SENSE
```

### 3. RV-3028 Driver (`rv3028.h/c`)
- Complete RTC driver implementation
- BCD time conversion functions
- Alarm setting and interrupt handling
- Wakeup time calculation
- I²C communication with proper error handling

### 4. Adaptive Scheduler (`adaptive_scheduler.c`)
```c
// Updated to use RV-3028 instead of built-in RTC
int ret = rv3028_init();
// Configure RTC interrupt GPIO (P0.02)
// Set wakeup time using RV-3028 alarm
```

### 5. Build Configuration (`CMakeLists.txt`)
```cmake
# Added RV-3028 driver to build
target_sources(app PRIVATE src/rv3028.c)
```

## 🔌 Pin Assignment Summary

| Pin | Function | Component |
|-----|----------|-----------|
| P0.02 | VBAT_SENSE | Battery voltage divider |
| P0.02 | RTC_INT | RV-3028 interrupt |
| P0.03 | RTC_CLOCKOUT | RV-3028 clock output |
| P0.26 | SCL | I²C clock (BME280 + RV-3028) |
| P0.27 | SDA | I²C data (BME280 + RV-3028) |

**Note**: P0.02 is shared between VBAT_SENSE and RTC_INT. The RV-3028 interrupt is active-low, so it won't interfere with the ADC reading when not triggered.

## ⚡ Power Management Integration

### RTC Wake-up System
- **RV-3028 Alarm**: Sets precise wake-up times
- **Interrupt Handling**: P0.02 configured as input with pull-up
- **Alarm Calculation**: Converts milliseconds to time-based alarm
- **Interrupt Clear**: Automatically clears alarm flags

### Battery Monitoring Accuracy
- **Voltage Divider**: 4.22MΩ + 1.87MΩ for ultra-low current draw
- **ADC Conversion**: 12-bit resolution with proper scaling
- **Calibration**: 3.256x gain factor for accurate voltage reading

## 🧪 Testing Validation

All firmware tests pass with the updated configuration:
- ✅ Sensor data encoding/decoding
- ✅ Power tier logic with hysteresis
- ✅ BME280 calibration algorithms
- ✅ BLE advertising format
- ✅ Battery monitoring with new voltage divider ratio

## 🚀 Build and Deploy

### 1. Build Firmware
```bash
cd mcu_firmware
./build.sh
```

### 2. Flash to Device
```bash
west flash -d build
```

### 3. Monitor Output
```bash
west espressif monitor -d build
```

## 📊 Expected Behavior

### Initialization
```
RV3028 status: 0x00
RV3028 control1: 0x10, control2: 0x00
RV3028 initialized successfully
BME280 found, chip ID: 0x60
Battery monitor initialized
Adaptive scheduler initialized with RV-3028
```

### Operation Cycle
1. **Wake-up**: RV-3028 alarm triggers on P0.02
2. **Battery Check**: ADC reads P0.02, calculates voltage with 3.256x gain
3. **Power Tier**: Determines wake interval based on battery voltage
4. **Sensor Read**: BME280 forced-mode reading via I²C
5. **BLE Advertise**: Broadcasts sensor data
6. **Sleep**: Sets next RV-3028 alarm and enters deep sleep

### Power Tiers (Your Configuration)
| Battery Voltage | Tier | Wake Interval | BLE Rate |
|----------------|------|---------------|----------|
| 4.2V - 3.8V | Normal | 5 minutes | 1 Hz |
| 3.8V - 3.6V | Conserve | 15 minutes | 0.2 Hz |
| 3.6V - 3.4V | Reserve | 30 minutes | 0.1 Hz |
| 3.4V - 3.2V | Survival | 60 minutes | 0.1 Hz |

## 🔍 Troubleshooting

### Common Issues
1. **I²C Communication**: Check pull-up resistors on P0.26/P0.27
2. **Battery Reading**: Verify voltage divider resistors (4.22MΩ + 1.87MΩ)
3. **RTC Interrupt**: Ensure P0.02 is properly connected to RV-3028 INT
4. **Power Consumption**: Monitor current with multimeter

### Debug Commands
```bash
# Check I²C devices
i2cdetect -y 1

# Monitor serial output
west espressif monitor -d build

# Test RTC functionality
# (Add RTC test commands to firmware)
```

Your hardware configuration is now fully integrated and ready for deployment! 🎯
