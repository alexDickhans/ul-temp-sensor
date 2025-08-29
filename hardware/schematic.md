# Hardware Schematic - Adaptive BLE Sensor Node

## Component List

### Core Components
- **MCU**: Raytac MDBT50Q (nRF52840 inside)
- **Sensor**: Bosch BME280 (I²C)
- **Battery**: Li-Po pouch cell (3.7V, 1000mAh+)
- **PMIC**: BQ25570 (solar charging)
- **Solar Panel**: 5V, 100mA+ (optional)

### Supporting Components
- **Voltage Divider**: 2x 100kΩ resistors for battery monitoring
- **Decoupling Capacitors**: 100nF ceramic + 10µF electrolytic
- **Pull-up Resistors**: 4.7kΩ for I²C lines
- **LED**: Status indicator (optional)
- **Switch**: Power on/off (optional)

## Wiring Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Sensor Node Schematic                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Battery (3.7V)                                             │
│  ┌─────────┐                                                │
│  │ Li-Po   │                                                │
│  │ Cell    │                                                │
│  └────┬────┘                                                │
│       │                                                     │
│       │  ┌─────────┐                                        │
│       └──┤ BQ25570 │                                        │
│          │ PMIC    │                                        │
│          └────┬────┘                                        │
│               │                                             │
│               │ 3.3V                                        │
│               │                                             │
│  ┌────────────┴────────────┐                                │
│  │                         │                                │
│  │    nRF52840 (MDBT50Q)   │                                │
│  │                         │                                │
│  │  ┌─────────┐            │                                │
│  │  │ VDD     │◄───────────┘                                │
│  │  │ GND     │                                             │
│  │  │ P0.26   │◄─── ADC (Battery Monitor)                   │
│  │  │ P0.27   │◄─── SDA (I²C)                               │
│  │  │ P0.26   │◄─── SCL (I²C)                               │
│  │  │ P0.30   │◄─── RTC (External)                          │
│  │  └─────────┘            │                                │
│  └─────────────────────────┘                                │
│               │                                             │
│               │ I²C Bus                                     │
│               │                                             │
│  ┌────────────┴────────────┐                                │
│  │                         │                                │
│  │      BME280 Sensor      │                                │
│  │                         │                                │
│  │  ┌─────────┐            │                                │
│  │  │ VCC     │◄───────────┘                                │
│  │  │ GND     │                                             │
│  │  │ SDA     │◄─── P0.27                                   │
│  │  │ SCL     │◄─── P0.26                                   │
│  │  │ ADDR    │◄─── GND (0x76) or VCC (0x77)               │
│  │  └─────────┘            │                                │
│  └─────────────────────────┘                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Detailed Connections

### Power Supply
```
Battery (3.7V) ──► BQ25570 VIN
BQ25570 VBAT ──► Battery +
BQ25570 VOUT ──► nRF52840 VDD (3.3V)
BQ25570 GND ──► nRF52840 GND
```

### Battery Monitoring
```
Battery + ──► 4.22MΩ ──► 1.87MΩ ──► GND
                    │
                    └──► nRF52840 P0.02 (ADC)
```

### I²C Bus (BME280 + RV-3028)
```
nRF52840 P0.27 ──► 4.7kΩ ──► 3.3V (SDA)
nRF52840 P0.26 ──► 4.7kΩ ──► 3.3V (SCL)
nRF52840 P0.27 ──► BME280 SDA
nRF52840 P0.26 ──► BME280 SCL
nRF52840 P0.27 ──► RV-3028 SDA
nRF52840 P0.26 ──► RV-3028 SCL
BME280 VCC ──► 3.3V
BME280 GND ──► GND
BME280 ADDR ──► GND (for 0x76 address)
RV-3028 VCC ──► 3.3V
RV-3028 GND ──► GND
RV-3028 INT ──► nRF52840 P0.02 (RTC interrupt)
RV-3028 CLKOUT ──► nRF52840 P0.03 (RTC clock output)
```

### Decoupling Capacitors
```
3.3V ──► 100nF ──► GND (close to nRF52840)
3.3V ──► 10µF ──► GND (power supply)
```

## PCB Layout Considerations

### Power Distribution
- Use thick traces for power lines (≥0.5mm)
- Place decoupling capacitors close to ICs
- Separate analog and digital grounds
- Use ground plane for better EMI performance

### I²C Bus
- Keep SDA and SCL traces close together
- Minimize trace length to reduce capacitance
- Place pull-up resistors close to devices
- Avoid routing near switching signals

### Battery Monitoring
- Use precision resistors for voltage divider
- Place ADC input close to voltage divider
- Add low-pass filter if needed (RC network)

### RF Considerations
- Keep antenna area clear of ground plane
- Minimize traces under antenna
- Use proper impedance matching (50Ω)
- Add ferrite beads on power lines if needed

## Component Specifications

### Raytac MDBT50Q
- **Operating Voltage**: 1.8V - 3.6V
- **Current Consumption**: 
  - Active: ~5mA
  - Sleep: <3µA
- **RF Output Power**: +8dBm max
- **Package**: QFN48 (6x6mm)

### BME280
- **Operating Voltage**: 1.71V - 3.6V
- **I²C Address**: 0x76 or 0x77
- **Temperature Range**: -40°C to +85°C
- **Pressure Range**: 300hPa to 1100hPa
- **Humidity Range**: 0% to 100%
- **Package**: LGA8 (2.5x2.5mm)

### RV-3028-C7 RTC
- **Operating Voltage**: 1.2V - 5.5V
- **I²C Address**: 0x52
- **Temperature Range**: -40°C to +85°C
- **Accuracy**: ±3.4 ppm at 25°C
- **Alarm Function**: Programmable alarm with interrupt
- **Package**: SOIC8 (3.9x4.9mm)

### BQ25570 PMIC
- **Input Voltage**: 0.5V - 5.5V
- **Output Voltage**: 3.3V (programmable)
- **Charging Current**: 100mA max
- **Package**: QFN20 (4x4mm)

### Li-Po Battery
- **Nominal Voltage**: 3.7V
- **Capacity**: 1000mAh minimum
- **Charging Voltage**: 4.2V
- **Discharge Cutoff**: 3.0V

## Assembly Instructions

### 1. Power Supply
1. Solder BQ25570 PMIC
2. Connect battery to PMIC input
3. Add decoupling capacitors
4. Test output voltage (should be 3.3V)

### 2. MCU
1. Solder nRF52840 (MDBT50Q)
2. Connect power and ground
3. Add decoupling capacitors
4. Test basic functionality

### 3. Sensor
1. Solder BME280 sensor
2. Connect I²C lines with pull-ups
3. Set address jumper (GND for 0x76)
4. Test I²C communication

### 4. Battery Monitor
1. Solder voltage divider resistors
2. Connect to ADC input
3. Calibrate voltage reading
4. Test battery monitoring

### 5. Final Assembly
1. Add status LED (optional)
2. Add power switch (optional)
3. Enclose in suitable housing
4. Test complete functionality

## Testing Checklist

- [ ] Power supply provides stable 3.3V
- [ ] nRF52840 boots and runs firmware
- [ ] BME280 responds to I²C commands
- [ ] Battery voltage reading is accurate
- [ ] BLE advertising works
- [ ] Deep sleep current <3µA
- [ ] RTC wake-up functions correctly
- [ ] All power tiers work as expected

## Troubleshooting

### Common Issues
1. **No power**: Check battery connection and PMIC
2. **I²C not working**: Check pull-up resistors and address
3. **High current**: Check for shorts or incorrect connections
4. **BLE range poor**: Check antenna area and impedance
5. **ADC inaccurate**: Check voltage divider values

### Debug Points
- Add test points for power rails
- Add LED indicators for status
- Use oscilloscope for I²C debugging
- Use multimeter for current measurement
