#!/bin/bash

# Build script for Adaptive BLE Sensor Node
# Requires nRF Connect SDK to be installed and sourced

set -e

# Configuration
BOARD="nrf52840dk_nrf52840"
BUILD_DIR="build"
CONFIG_FILE="prj.conf"

echo "Building Adaptive BLE Sensor Node for $BOARD..."

# Create build directory
mkdir -p $BUILD_DIR

# Build the project
west build -b $BOARD -d $BUILD_DIR -- -DCONF_FILE=$CONFIG_FILE

echo "Build completed successfully!"
echo "Binary location: $BUILD_DIR/zephyr/zephyr.hex"
echo ""
echo "To flash the device:"
echo "west flash -d $BUILD_DIR"
echo ""
echo "To monitor serial output:"
echo "west espressif monitor -d $BUILD_DIR"
