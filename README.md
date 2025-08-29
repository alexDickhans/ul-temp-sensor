# Adaptive BLE Sensor Node System

A complete IoT sensor system featuring adaptive power management, BLE communication, and data logging. The system consists of a Raytac MDBT50Q (nRF52840) sensor node and a Raspberry Pi host application.

## 🏗️ System Architecture

```
┌─────────────────┐    BLE Advertisement    ┌─────────────────┐
│   Sensor Node   │ ──────────────────────► │  Raspberry Pi   │
│                 │                         │     Host        │
│ • nRF52840      │                         │                 │
│ • BME280        │                         │ • BLE Scanner   │
│ • Battery       │                         │ • Data Logger   │
│ • Adaptive PM   │                         │ • SQLite DB     │
└─────────────────┘                         └─────────────────┘
```

## 📋 Features

### MCU Firmware (nRF52840)
- **Adaptive Power Management**: 4-tier power system based on battery voltage
- **BME280 Integration**: Temperature, pressure, and humidity sensing
- **BLE Advertising**: Non-connectable advertisements with sensor data
- **Deep Sleep**: System OFF mode with RTC wake-up
- **Battery Monitoring**: ADC-based voltage sensing

### Raspberry Pi Host
- **BLE Scanner**: Continuous scanning for sensor advertisements
- **Data Decoding**: Parses manufacturer-specific data payloads
- **SQLite Database**: Local data storage with indexing
- **Data Viewer**: Query and display sensor data
- **Systemd Service**: Automatic startup and management

## 🔋 Power Management Tiers

| Tier | Battery Range | Wake Interval | BLE Rate | Description |
|------|---------------|---------------|----------|-------------|
| Normal | 4.2V - 3.8V | 5 minutes | 1 Hz | Full performance |
| Conserve | 3.8V - 3.6V | 15 minutes | 0.2 Hz | Reduced frequency |
| Reserve | 3.6V - 3.4V | 30 minutes | 0.1 Hz | Minimal operation |
| Survival | 3.4V - 3.2V | 60 minutes | 0.1 Hz | Critical battery |

## 📦 Hardware Requirements

### Sensor Node
- **MCU**: Raytac MDBT50Q (nRF52840 inside)
- **Sensor**: Bosch BME280 (I²C)
- **Battery**: Li-Po pouch (3.7V, 1000mAh+)
- **PMIC**: BQ25570 (solar charging)
- **Connections**: I²C, ADC, RTC

### Host System
- **Platform**: Raspberry Pi (any modern Pi with BLE)
- **OS**: Raspberry Pi OS or Ubuntu
- **Storage**: SD card with 8GB+ free space

## 🚀 Quick Start

### 1. MCU Firmware Setup

```bash
# Install nRF Connect SDK
# Follow: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html

# Clone and build
cd mcu_firmware
./build.sh

# Flash to device
west flash -d build
```

### 2. Raspberry Pi Host Setup

```bash
# Install dependencies
sudo apt update
sudo apt install python3-pip python3-venv bluetooth bluez

# Create virtual environment
cd pi_host
python3 -m venv venv
source venv/bin/activate

# Install Python packages
pip install -r requirements.txt

# Run scanner
python3 sensor_scanner.py
```

### 3. View Data

```bash
# List known devices
python3 data_viewer.py --list-devices

# View recent data
python3 data_viewer.py --hours 24 --stats

# Filter by device
python3 data_viewer.py --device AA:BB:CC:DD:EE:FF
```

## 📊 BLE Data Format

The sensor node broadcasts manufacturer-specific data with the following structure:

```c
struct sensor_adv_data {
    uint8_t version;           // Protocol version (1)
    uint8_t tier;              // Power tier (0-3)
    uint16_t battery_mv;       // Battery voltage in mV
    int16_t temperature;       // Temperature * 100 (°C)
    uint16_t pressure;         // Pressure * 10 (hPa)
    uint16_t humidity;         // Humidity * 100 (%)
    uint32_t timestamp;        // Unix timestamp
};
```

**Company ID**: Nordic Semiconductor (0x0059)

## 🔧 Configuration

### MCU Configuration

Edit `mcu_firmware/prj.conf` for:
- BLE power settings
- I²C configuration
- ADC settings
- Power management options

### Host Configuration

Command-line options for `sensor_scanner.py`:
- `--db`: Database file path
- `--scan-duration`: BLE scan duration (seconds)
- `--log-level`: Logging verbosity

## 📈 Data Analysis

### Database Schema

```sql
CREATE TABLE sensor_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_address TEXT NOT NULL,
    device_name TEXT,
    timestamp DATETIME NOT NULL,
    temperature REAL,
    pressure REAL,
    humidity REAL,
    battery_mv INTEGER,
    power_tier INTEGER,
    rssi INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

### Example Queries

```sql
-- Average temperature by hour
SELECT strftime('%Y-%m-%d %H:00:00', timestamp) as hour,
       AVG(temperature) as avg_temp
FROM sensor_data 
GROUP BY hour 
ORDER BY hour DESC;

-- Battery status over time
SELECT timestamp, battery_mv, power_tier
FROM sensor_data 
WHERE device_address = 'AA:BB:CC:DD:EE:FF'
ORDER BY timestamp DESC;
```

## 🔍 Troubleshooting

### Common Issues

1. **No BLE advertisements detected**
   - Check device is powered and firmware is flashed
   - Verify BLE is enabled on Raspberry Pi
   - Check for interference or range issues

2. **High power consumption**
   - Verify deep sleep is working (measure current <3µA)
   - Check BLE advertising intervals
   - Monitor battery voltage thresholds

3. **Sensor reading errors**
   - Check I²C connections
   - Verify BME280 address (0x76 or 0x77)
   - Check power supply stability

### Debug Commands

```bash
# Monitor MCU serial output
west espressif monitor -d build

# Check BLE devices
sudo hcitool lescan

# Monitor system logs
journalctl -u sensor-scanner -f

# Check database integrity
sqlite3 sensor_data.db "PRAGMA integrity_check;"
```

## 🔄 Systemd Service

Create `/etc/systemd/system/sensor-scanner.service`:

```ini
[Unit]
Description=BLE Sensor Scanner
After=bluetooth.service
Wants=bluetooth.service

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/sensor-system/pi_host
Environment=PATH=/home/pi/sensor-system/pi_host/venv/bin
ExecStart=/home/pi/sensor-system/pi_host/venv/bin/python sensor_scanner.py
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable sensor-scanner
sudo systemctl start sensor-scanner
```

## 📚 Development

### Project Structure

```
temp-sensor-code/
├── mcu_firmware/           # nRF52840 firmware
│   ├── src/               # Source files
│   ├── boards/            # Device tree overlays
│   ├── prj.conf          # Zephyr configuration
│   └── build.sh          # Build script
├── pi_host/              # Raspberry Pi application
│   ├── sensor_scanner.py # Main scanner
│   ├── data_viewer.py    # Data query tool
│   └── requirements.txt  # Python dependencies
└── README.md             # This file
```

### Adding New Sensors

1. Extend the `sensor_adv_data` structure
2. Update the Python decoder
3. Modify database schema
4. Update data viewer

### Power Optimization

- Use forced mode for BME280 (not continuous)
- Minimize BLE advertising duration
- Optimize RTC wake intervals
- Monitor current consumption with multimeter

## 📄 License

This project is open source. See LICENSE file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For issues and questions:
- Check the troubleshooting section
- Review the code comments
- Open an issue on GitHub

---

**Note**: This system is designed for educational and prototyping purposes. For production use, consider additional security measures and environmental protection.
