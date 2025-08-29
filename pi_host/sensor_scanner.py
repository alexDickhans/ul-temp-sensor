#!/usr/bin/env python3
"""
Adaptive BLE Sensor Node - Raspberry Pi Host Application

This script scans for BLE advertisements from temperature sensor nodes,
decodes the sensor data, and logs it to a database.
"""

import asyncio
import logging
import sqlite3
import argparse
import time
from datetime import datetime
from typing import Optional, Dict, Any
from dataclasses import dataclass
import struct

from bleak import BleakScanner
from bleak.backends.scanner import AdvertisementData
from bleak.backends.device import BLEDevice

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class SensorData:
    """Container for decoded sensor data"""
    device_address: str
    device_name: str
    timestamp: datetime
    temperature: float
    pressure: float
    humidity: float
    battery_mv: int
    power_tier: int
    rssi: int

class SensorDataDecoder:
    """Decodes BLE advertisement data from sensor nodes"""
    
    # Nordic Semiconductor Company ID
    NORDIC_COMPANY_ID = 0x0059
    
    @staticmethod
    def decode_manufacturer_data(data: bytes) -> Optional[Dict[str, Any]]:
        """Decode manufacturer-specific data from BLE advertisement"""
        if len(data) < 6:  # Minimum length: 2 bytes company ID + 4 bytes payload
            return None
            
        # Check company ID (little endian)
        company_id = struct.unpack('<H', data[0:2])[0]
        if company_id != SensorDataDecoder.NORDIC_COMPANY_ID:
            return None
            
        # Extract sensor payload
        payload = data[2:]
        if len(payload) < 12:  # Minimum payload size
            return None
            
        try:
            # Decode payload structure
            # struct sensor_adv_data {
            #     uint8_t version;           // 1 byte
            #     uint8_t tier;              // 1 byte
            #     uint16_t battery_mv;       // 2 bytes
            #     int16_t temperature;       // 2 bytes (temp * 100)
            #     uint16_t pressure;         // 2 bytes (pressure * 10)
            #     uint16_t humidity;         // 2 bytes (humidity * 100)
            #     uint32_t timestamp;        // 4 bytes
            # }
            
            version, tier, battery_mv, temp_raw, pressure_raw, humidity_raw, timestamp = \
                struct.unpack('<BBHhhHI', payload[:12])
                
            # Convert raw values to actual measurements
            temperature = temp_raw / 100.0
            pressure = pressure_raw / 10.0
            humidity = humidity_raw / 100.0
            
            return {
                'version': version,
                'tier': tier,
                'battery_mv': battery_mv,
                'temperature': temperature,
                'pressure': pressure,
                'humidity': humidity,
                'timestamp': timestamp
            }
            
        except struct.error as e:
            logger.warning(f"Failed to decode payload: {e}")
            return None

class SensorDatabase:
    """SQLite database for storing sensor data"""
    
    def __init__(self, db_path: str):
        self.db_path = db_path
        self.init_database()
    
    def init_database(self):
        """Initialize the database with required tables"""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            
            # Create sensor_data table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS sensor_data (
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
                )
            ''')
            
            # Create index for efficient queries
            cursor.execute('''
                CREATE INDEX IF NOT EXISTS idx_device_timestamp 
                ON sensor_data(device_address, timestamp)
            ''')
            
            conn.commit()
            logger.info(f"Database initialized: {self.db_path}")
    
    def insert_sensor_data(self, data: SensorData):
        """Insert sensor data into the database"""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            
            cursor.execute('''
                INSERT INTO sensor_data 
                (device_address, device_name, timestamp, temperature, pressure, 
                 humidity, battery_mv, power_tier, rssi)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', (
                data.device_address,
                data.device_name,
                data.timestamp.isoformat(),
                data.temperature,
                data.pressure,
                data.humidity,
                data.battery_mv,
                data.power_tier,
                data.rssi
            ))
            
            conn.commit()
            logger.debug(f"Data inserted for device {data.device_address}")

class BLESensorScanner:
    """Main BLE scanner for sensor nodes"""
    
    def __init__(self, db_path: str, scan_duration: int = 30):
        self.db = SensorDatabase(db_path)
        self.scan_duration = scan_duration
        self.decoder = SensorDataDecoder()
        self.known_devices = set()
        
    def advertisement_callback(self, device: BLEDevice, advertisement_data: AdvertisementData):
        """Callback for BLE advertisement detection"""
        try:
            # Check if this is a sensor device
            if not self._is_sensor_device(device, advertisement_data):
                return
                
            # Decode manufacturer data
            if advertisement_data.manufacturer_data:
                for company_id, data in advertisement_data.manufacturer_data.items():
                    if company_id == self.decoder.NORDIC_COMPANY_ID:
                        decoded_data = self.decoder.decode_manufacturer_data(data)
                        if decoded_data:
                            self._process_sensor_data(device, advertisement_data, decoded_data)
                        break
                        
        except Exception as e:
            logger.error(f"Error processing advertisement from {device.address}: {e}")
    
    def _is_sensor_device(self, device: BLEDevice, advertisement_data: AdvertisementData) -> bool:
        """Check if the device is a sensor node"""
        # Check device name
        if advertisement_data.local_name and "TempSensor" in advertisement_data.local_name:
            return True
            
        # Check if we've seen this device before
        if device.address in self.known_devices:
            return True
            
        # Check for Nordic manufacturer data
        if advertisement_data.manufacturer_data:
            for company_id in advertisement_data.manufacturer_data.keys():
                if company_id == self.decoder.NORDIC_COMPANY_ID:
                    self.known_devices.add(device.address)
                    return True
                    
        return False
    
    def _process_sensor_data(self, device: BLEDevice, advertisement_data: AdvertisementData, 
                           decoded_data: Dict[str, Any]):
        """Process decoded sensor data"""
        try:
            # Create sensor data object
            sensor_data = SensorData(
                device_address=device.address,
                device_name=advertisement_data.local_name or "Unknown",
                timestamp=datetime.now(),
                temperature=decoded_data['temperature'],
                pressure=decoded_data['pressure'],
                humidity=decoded_data['humidity'],
                battery_mv=decoded_data['battery_mv'],
                power_tier=decoded_data['tier'],
                rssi=advertisement_data.rssi
            )
            
            # Log the data
            logger.info(
                f"Sensor: {device.address} | "
                f"T: {sensor_data.temperature:.2f}°C | "
                f"P: {sensor_data.pressure:.1f} hPa | "
                f"H: {sensor_data.humidity:.2f}% | "
                f"Battery: {sensor_data.battery_mv} mV | "
                f"Tier: {sensor_data.power_tier} | "
                f"RSSI: {sensor_data.rssi} dBm"
            )
            
            # Store in database
            self.db.insert_sensor_data(sensor_data)
            
        except Exception as e:
            logger.error(f"Error processing sensor data: {e}")
    
    async def start_scanning(self):
        """Start continuous BLE scanning"""
        logger.info(f"Starting BLE scanner (scan duration: {self.scan_duration}s)")
        
        while True:
            try:
                # Scan for devices
                devices = await BleakScanner.discover(
                    timeout=self.scan_duration,
                    detection_callback=self.advertisement_callback
                )
                
                logger.debug(f"Scan completed, found {len(devices)} devices")
                
                # Brief pause between scans
                await asyncio.sleep(1)
                
            except Exception as e:
                logger.error(f"Scan error: {e}")
                await asyncio.sleep(5)  # Wait before retrying

async def main():
    """Main application entry point"""
    parser = argparse.ArgumentParser(description='BLE Sensor Scanner')
    parser.add_argument('--db', default='sensor_data.db', help='Database file path')
    parser.add_argument('--scan-duration', type=int, default=30, help='Scan duration in seconds')
    parser.add_argument('--log-level', default='INFO', help='Logging level')
    
    args = parser.parse_args()
    
    # Set log level
    logging.getLogger().setLevel(getattr(logging, args.log_level.upper()))
    
    # Create scanner
    scanner = BLESensorScanner(args.db, args.scan_duration)
    
    logger.info("Adaptive BLE Sensor Scanner Starting...")
    logger.info(f"Database: {args.db}")
    logger.info(f"Scan duration: {args.scan_duration}s")
    
    try:
        await scanner.start_scanning()
    except KeyboardInterrupt:
        logger.info("Scanner stopped by user")
    except Exception as e:
        logger.error(f"Scanner error: {e}")

if __name__ == "__main__":
    asyncio.run(main())
