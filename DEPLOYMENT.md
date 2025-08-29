# Deployment Guide - Adaptive BLE Sensor Node System

This guide provides step-by-step instructions for deploying the complete adaptive BLE sensor node system.

## 📋 Prerequisites

### Hardware Requirements
- **Sensor Node**: Raytac MDBT50Q development board or custom PCB
- **Host System**: Raspberry Pi (3B+, 4, or newer)
- **BME280 Sensor**: Breakout board or module
- **Battery**: Li-Po cell (3.7V, 1000mAh+)
- **PMIC**: BQ25570 (optional, for solar charging)
- **Cables**: Jumper wires, USB cables

### Software Requirements
- **nRF Connect SDK**: v2.0+ for MCU development
- **Raspberry Pi OS**: Latest version with desktop
- **Python 3.8+**: For host application
- **Git**: For source code management

## 🚀 Step 1: MCU Firmware Development

### 1.1 Install nRF Connect SDK

```bash
# Follow official installation guide:
# https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html

# For Ubuntu/Debian:
wget https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-18-1/nrf-command-line-tools_10.18.1_amd64.deb
sudo dpkg -i nrf-command-line-tools_10.18.1_amd64.deb

# Install west
pip3 install west

# Initialize workspace
west init -m https://github.com/nrfconnect/sdk-nrf --mr v2.0.0
west update
west zephyr-export
```

### 1.2 Build and Flash Firmware

```bash
# Clone the project
git clone <your-repo-url>
cd temp-sensor-code

# Build firmware
cd mcu_firmware
./build.sh

# Flash to device (replace with your device)
west flash -d build

# Monitor serial output
west espressif monitor -d build
```

### 1.3 Verify Firmware Operation

1. **Check Serial Output**: Should see initialization messages
2. **Test BLE Advertising**: Use phone app or `hcitool lescan`
3. **Verify Sensor Reading**: Check temperature/pressure values
4. **Test Power Management**: Monitor current consumption

## 🖥️ Step 2: Raspberry Pi Host Setup

### 2.1 Prepare Raspberry Pi

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install -y python3-pip python3-venv bluetooth bluez git

# Enable Bluetooth
sudo systemctl enable bluetooth
sudo systemctl start bluetooth
```

### 2.2 Install Host Application

```bash
# Clone project
git clone <your-repo-url>
cd temp-sensor-code

# Run installation script
cd pi_host
./install.sh
```

### 2.3 Verify Installation

```bash
# Check service status
sudo systemctl status sensor-scanner

# View logs
sudo journalctl -u sensor-scanner -f

# Test data viewer
python3 data_viewer.py --list-devices
```

## 🔧 Step 3: Hardware Assembly

### 3.1 Sensor Node Assembly

#### Option A: Development Board (Quick Start)
1. Connect BME280 to I²C pins (SDA: P0.27, SCL: P0.26)
2. Add voltage divider for battery monitoring
3. Connect battery to power input
4. Test basic functionality

#### Option B: Custom PCB (Production)
1. Follow hardware schematic in `hardware/schematic.md`
2. Solder components in order:
   - PMIC (BQ25570)
   - MCU (nRF52840)
   - Sensor (BME280)
   - Supporting components
3. Test each stage before proceeding

### 3.2 Power Supply Setup

```bash
# Test battery voltage
# Should read 3.7V nominal, 4.2V when fully charged

# Test PMIC output
# Should provide stable 3.3V to MCU

# Measure current consumption
# Active: ~5mA, Sleep: <3µA
```

### 3.3 Enclosure and Mounting

1. **Choose Enclosure**: Weatherproof for outdoor use
2. **Mounting**: Secure to wall, pole, or surface
3. **Cable Management**: Route cables neatly
4. **Antenna Position**: Keep clear of metal objects

## 📊 Step 4: System Integration

### 4.1 Test End-to-End Communication

```bash
# Start sensor scanner
sudo systemctl start sensor-scanner

# Check for advertisements
sudo hcitool lescan

# View received data
python3 data_viewer.py --hours 1 --stats
```

### 4.2 Configure MQTT Bridge (Optional)

```bash
# Install MQTT bridge
pip3 install asyncio-mqtt

# Start MQTT bridge
python3 mqtt_bridge.py --mqtt-host localhost --topic-prefix sensors

# For Home Assistant integration
# Add MQTT integration in Home Assistant
# Configure sensors using discovery topics
```

### 4.3 Set Up Monitoring

```bash
# Create monitoring script
cat > /home/pi/monitor.sh << 'EOF'
#!/bin/bash
# Check sensor scanner status
if ! systemctl is-active --quiet sensor-scanner; then
    echo "Sensor scanner is down, restarting..."
    sudo systemctl restart sensor-scanner
fi

# Check database size
db_size=$(du -h /home/pi/sensor_data.db | cut -f1)
echo "Database size: $db_size"

# Check recent data
python3 /home/pi/temp-sensor-code/pi_host/data_viewer.py --hours 1 --limit 5
EOF

chmod +x /home/pi/monitor.sh

# Add to crontab
(crontab -l 2>/dev/null; echo "*/5 * * * * /home/pi/monitor.sh") | crontab -
```

## 🔍 Step 5: Testing and Validation

### 5.1 Functional Testing

```bash
# Run firmware tests
cd testing
python3 test_firmware.py

# Test power management
# Monitor current consumption over time
# Verify tier transitions

# Test sensor accuracy
# Compare with reference thermometer
# Check pressure against weather data
```

### 5.2 Performance Testing

```bash
# Test BLE range
# Measure RSSI at different distances
# Test through walls/obstacles

# Test battery life
# Monitor voltage over time
# Calculate runtime estimates

# Test data reliability
# Check packet loss rates
# Verify data integrity
```

### 5.3 Environmental Testing

```bash
# Temperature testing
# Test in hot/cold environments
# Verify sensor accuracy

# Humidity testing
# Test in high/low humidity
# Check sensor response

# Outdoor testing
# Test weather resistance
# Verify solar charging (if applicable)
```

## 📈 Step 6: Production Deployment

### 6.1 Multiple Sensor Deployment

```bash
# Deploy multiple sensors
# Use different device names
# Monitor all devices

# Create deployment script
cat > deploy_sensors.sh << 'EOF'
#!/bin/bash
# Script to deploy multiple sensors
for i in {1..5}; do
    echo "Deploying sensor $i..."
    # Flash firmware with unique ID
    # Configure device name
    # Test functionality
done
EOF
```

### 6.2 Network Configuration

```bash
# Configure static IP for Raspberry Pi
sudo nano /etc/dhcpcd.conf

# Add to end of file:
interface eth0
static ip_address=192.168.1.100/24
static routers=192.168.1.1
static domain_name_servers=8.8.8.8

# Restart networking
sudo systemctl restart dhcpcd
```

### 6.3 Backup and Recovery

```bash
# Create backup script
cat > backup_system.sh << 'EOF'
#!/bin/bash
# Backup database
cp /home/pi/sensor_data.db /home/pi/backup/sensor_data_$(date +%Y%m%d_%H%M%S).db

# Backup configuration
tar -czf /home/pi/backup/config_$(date +%Y%m%d_%H%M%S).tar.gz \
    /home/pi/temp-sensor-code/pi_host/

# Clean old backups (keep last 7 days)
find /home/pi/backup -name "*.db" -mtime +7 -delete
find /home/pi/backup -name "*.tar.gz" -mtime +7 -delete
EOF

chmod +x backup_system.sh

# Add to crontab (daily backup)
(crontab -l 2>/dev/null; echo "0 2 * * * /home/pi/backup_system.sh") | crontab -
```

## 🔧 Step 7: Maintenance and Updates

### 7.1 Regular Maintenance

```bash
# Weekly maintenance tasks
# Update system packages
sudo apt update && sudo apt upgrade -y

# Check service status
sudo systemctl status sensor-scanner

# Monitor disk space
df -h

# Check log files
sudo journalctl --disk-usage
```

### 7.2 Firmware Updates

```bash
# Update MCU firmware
cd mcu_firmware
git pull
./build.sh
west flash -d build

# Update host application
cd pi_host
git pull
sudo systemctl restart sensor-scanner
```

### 7.3 Troubleshooting

```bash
# Common issues and solutions

# BLE not working
sudo systemctl restart bluetooth
sudo hciconfig hci0 up

# Sensor not responding
# Check I²C connections
i2cdetect -y 1

# High power consumption
# Check for stuck processes
ps aux | grep python

# Database issues
sqlite3 sensor_data.db "PRAGMA integrity_check;"
```

## 📊 Step 8: Monitoring and Analytics

### 8.1 Data Analysis

```bash
# Create analysis script
cat > analyze_data.py << 'EOF'
#!/usr/bin/env python3
import sqlite3
import pandas as pd
from datetime import datetime, timedelta

# Load data
conn = sqlite3.connect('/home/pi/sensor_data.db')
df = pd.read_sql_query("""
    SELECT * FROM sensor_data 
    WHERE timestamp >= datetime('now', '-7 days')
""", conn)

# Generate reports
print("Weekly Statistics:")
print(f"Total readings: {len(df)}")
print(f"Average temperature: {df['temperature'].mean():.2f}°C")
print(f"Battery status: {df['battery_mv'].mean():.0f} mV")

# Save to file
df.to_csv('/home/pi/reports/weekly_report.csv', index=False)
EOF

# Run weekly analysis
(crontab -l 2>/dev/null; echo "0 8 * * 1 python3 /home/pi/analyze_data.py") | crontab -
```

### 8.2 Alerting

```bash
# Create alert script
cat > check_alerts.py << 'EOF'
#!/usr/bin/env python3
import sqlite3
import smtplib
from email.mime.text import MIMEText

# Check for alerts
conn = sqlite3.connect('/home/pi/sensor_data.db')
cursor = conn.cursor()

# Low battery alert
cursor.execute("""
    SELECT device_address, battery_mv, timestamp 
    FROM sensor_data 
    WHERE battery_mv < 3500 
    AND timestamp >= datetime('now', '-1 hour')
""")

low_battery = cursor.fetchall()

if low_battery:
    # Send email alert
    msg = MIMEText(f"Low battery alert: {low_battery}")
    msg['Subject'] = 'Sensor Battery Alert'
    msg['From'] = 'sensor@example.com'
    msg['To'] = 'admin@example.com'
    
    # Configure SMTP and send
    # (Add your SMTP configuration here)
EOF
```

## ✅ Deployment Checklist

- [ ] MCU firmware compiled and flashed
- [ ] Sensor node hardware assembled and tested
- [ ] Raspberry Pi host application installed
- [ ] BLE communication verified
- [ ] Database receiving data
- [ ] Power management working correctly
- [ ] MQTT bridge configured (if needed)
- [ ] Monitoring and alerting set up
- [ ] Backup system configured
- [ ] Documentation updated

## 🆘 Support and Troubleshooting

### Getting Help
1. Check the troubleshooting section in README.md
2. Review system logs: `sudo journalctl -u sensor-scanner -f`
3. Test individual components
4. Check hardware connections
5. Verify power supply stability

### Common Issues
- **No BLE advertisements**: Check firmware, power, antenna
- **High power consumption**: Verify deep sleep, check for loops
- **Sensor reading errors**: Check I²C connections, verify address
- **Database issues**: Check permissions, disk space, SQLite integrity

### Performance Optimization
- Adjust BLE advertising intervals based on requirements
- Optimize sensor reading frequency
- Monitor and tune power management thresholds
- Use appropriate battery capacity for deployment duration
