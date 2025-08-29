#!/bin/bash

# Installation script for Adaptive BLE Sensor Scanner
# Run this script on your Raspberry Pi

set -e

echo "Installing Adaptive BLE Sensor Scanner..."

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo "This script should not be run as root. Please run as pi user."
   exit 1
fi

# Update system packages
echo "Updating system packages..."
sudo apt update
sudo apt install -y python3-pip python3-venv bluetooth bluez python3-dev

# Create project directory
PROJECT_DIR="$HOME/temp-sensor-code"
echo "Creating project directory: $PROJECT_DIR"
mkdir -p "$PROJECT_DIR"

# Copy files (assuming script is run from project root)
if [ -d "pi_host" ]; then
    cp -r pi_host "$PROJECT_DIR/"
else
    echo "Error: pi_host directory not found. Please run from project root."
    exit 1
fi

cd "$PROJECT_DIR/pi_host"

# Create virtual environment
echo "Creating Python virtual environment..."
python3 -m venv venv

# Activate virtual environment and install dependencies
echo "Installing Python dependencies..."
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

# Install systemd service
echo "Installing systemd service..."
sudo cp sensor-scanner.service /etc/systemd/system/
sudo systemctl daemon-reload

# Enable and start service
echo "Enabling and starting sensor scanner service..."
sudo systemctl enable sensor-scanner
sudo systemctl start sensor-scanner

# Check service status
echo "Checking service status..."
sudo systemctl status sensor-scanner --no-pager

echo ""
echo "Installation completed successfully!"
echo ""
echo "Useful commands:"
echo "  View service logs: sudo journalctl -u sensor-scanner -f"
echo "  Stop service: sudo systemctl stop sensor-scanner"
echo "  Start service: sudo systemctl start sensor-scanner"
echo "  Restart service: sudo systemctl restart sensor-scanner"
echo ""
echo "Data viewer commands:"
echo "  View recent data: $PROJECT_DIR/pi_host/venv/bin/python data_viewer.py"
echo "  List devices: $PROJECT_DIR/pi_host/venv/bin/python data_viewer.py --list-devices"
echo ""
echo "Database location: $HOME/sensor_data.db"
