#!/usr/bin/env python3
"""
MQTT Bridge for Sensor Data

Forwards sensor data from SQLite database to MQTT topics for integration
with Home Assistant, Node-RED, or other IoT platforms.
"""

import asyncio
import sqlite3
import json
import argparse
import logging
from datetime import datetime, timedelta
from typing import Dict, Any, Optional
import asyncio_mqtt as mqtt

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class MQTTBridge:
    """MQTT bridge for sensor data"""
    
    def __init__(self, db_path: str, mqtt_host: str, mqtt_port: int = 1883,
                 mqtt_username: Optional[str] = None, mqtt_password: Optional[str] = None,
                 topic_prefix: str = "sensors", publish_interval: int = 60):
        self.db_path = db_path
        self.mqtt_host = mqtt_host
        self.mqtt_port = mqtt_port
        self.mqtt_username = mqtt_username
        self.mqtt_password = mqtt_password
        self.topic_prefix = topic_prefix
        self.publish_interval = publish_interval
        self.last_published = {}  # Track last published timestamp per device
        
    def get_latest_sensor_data(self) -> Dict[str, Any]:
        """Get latest sensor data for each device"""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            
            # Get latest data for each device
            cursor.execute('''
                SELECT device_address, device_name, timestamp, temperature, 
                       pressure, humidity, battery_mv, power_tier, rssi
                FROM sensor_data s1
                WHERE timestamp = (
                    SELECT MAX(timestamp) 
                    FROM sensor_data s2 
                    WHERE s2.device_address = s1.device_address
                )
                ORDER BY device_address
            ''')
            
            devices = {}
            for row in cursor.fetchall():
                device_address, device_name, timestamp, temperature, pressure, \
                humidity, battery_mv, power_tier, rssi = row
                
                devices[device_address] = {
                    'device_name': device_name,
                    'timestamp': timestamp,
                    'temperature': temperature,
                    'pressure': pressure,
                    'humidity': humidity,
                    'battery_mv': battery_mv,
                    'power_tier': power_tier,
                    'rssi': rssi
                }
                
        return devices
    
    def get_new_sensor_data(self) -> Dict[str, Any]:
        """Get sensor data that hasn't been published yet"""
        devices = self.get_latest_sensor_data()
        new_data = {}
        
        for device_address, data in devices.items():
            last_published = self.last_published.get(device_address)
            if last_published is None or data['timestamp'] > last_published:
                new_data[device_address] = data
                
        return new_data
    
    async def publish_device_data(self, client: mqtt.Client, device_address: str, data: Dict[str, Any]):
        """Publish device data to MQTT topics"""
        try:
            # Create device-friendly name
            device_name = data['device_name'] or device_address.replace(':', '_')
            
            # Publish individual sensor values
            topics = {
                f"{self.topic_prefix}/{device_name}/temperature": data['temperature'],
                f"{self.topic_prefix}/{device_name}/pressure": data['pressure'],
                f"{self.topic_prefix}/{device_name}/humidity": data['humidity'],
                f"{self.topic_prefix}/{device_name}/battery": data['battery_mv'],
                f"{self.topic_prefix}/{device_name}/power_tier": data['power_tier'],
                f"{self.topic_prefix}/{device_name}/rssi": data['rssi'],
                f"{self.topic_prefix}/{device_name}/timestamp": data['timestamp']
            }
            
            for topic, value in topics.items():
                if value is not None:
                    payload = json.dumps({
                        'value': value,
                        'timestamp': data['timestamp'],
                        'device': device_address
                    })
                    await client.publish(topic, payload, qos=1)
                    logger.debug(f"Published {topic}: {payload}")
            
            # Publish complete device state
            state_topic = f"{self.topic_prefix}/{device_name}/state"
            state_payload = json.dumps({
                'device_address': device_address,
                'device_name': data['device_name'],
                'temperature': data['temperature'],
                'pressure': data['pressure'],
                'humidity': data['humidity'],
                'battery_mv': data['battery_mv'],
                'power_tier': data['power_tier'],
                'rssi': data['rssi'],
                'timestamp': data['timestamp']
            })
            await client.publish(state_topic, state_payload, qos=1)
            
            # Update last published timestamp
            self.last_published[device_address] = data['timestamp']
            logger.info(f"Published data for device {device_name}")
            
        except Exception as e:
            logger.error(f"Error publishing data for {device_address}: {e}")
    
    async def publish_discovery_info(self, client: mqtt.Client, device_address: str, data: Dict[str, Any]):
        """Publish Home Assistant discovery information"""
        try:
            device_name = data['device_name'] or device_address.replace(':', '_')
            device_id = device_address.replace(':', '_')
            
            # Temperature sensor
            temp_config = {
                'name': f"{device_name} Temperature",
                'unique_id': f"{device_id}_temperature",
                'device_class': 'temperature',
                'unit_of_measurement': '°C',
                'state_topic': f"{self.topic_prefix}/{device_name}/temperature",
                'value_template': '{{ value_json.value }}',
                'device': {
                    'identifiers': [device_address],
                    'name': device_name,
                    'manufacturer': 'Custom BLE Sensor'
                }
            }
            await client.publish(
                f"homeassistant/sensor/{device_id}_temperature/config",
                json.dumps(temp_config),
                qos=1,
                retain=True
            )
            
            # Humidity sensor
            hum_config = {
                'name': f"{device_name} Humidity",
                'unique_id': f"{device_id}_humidity",
                'device_class': 'humidity',
                'unit_of_measurement': '%',
                'state_topic': f"{self.topic_prefix}/{device_name}/humidity",
                'value_template': '{{ value_json.value }}',
                'device': {
                    'identifiers': [device_address],
                    'name': device_name,
                    'manufacturer': 'Custom BLE Sensor'
                }
            }
            await client.publish(
                f"homeassistant/sensor/{device_id}_humidity/config",
                json.dumps(hum_config),
                qos=1,
                retain=True
            )
            
            # Pressure sensor
            press_config = {
                'name': f"{device_name} Pressure",
                'unique_id': f"{device_id}_pressure",
                'device_class': 'pressure',
                'unit_of_measurement': 'hPa',
                'state_topic': f"{self.topic_prefix}/{device_name}/pressure",
                'value_template': '{{ value_json.value }}',
                'device': {
                    'identifiers': [device_address],
                    'name': device_name,
                    'manufacturer': 'Custom BLE Sensor'
                }
            }
            await client.publish(
                f"homeassistant/sensor/{device_id}_pressure/config",
                json.dumps(press_config),
                qos=1,
                retain=True
            )
            
            # Battery sensor
            battery_config = {
                'name': f"{device_name} Battery",
                'unique_id': f"{device_id}_battery",
                'device_class': 'battery',
                'unit_of_measurement': 'mV',
                'state_topic': f"{self.topic_prefix}/{device_name}/battery",
                'value_template': '{{ value_json.value }}',
                'device': {
                    'identifiers': [device_address],
                    'name': device_name,
                    'manufacturer': 'Custom BLE Sensor'
                }
            }
            await client.publish(
                f"homeassistant/sensor/{device_id}_battery/config",
                json.dumps(battery_config),
                qos=1,
                retain=True
            )
            
            logger.info(f"Published discovery info for {device_name}")
            
        except Exception as e:
            logger.error(f"Error publishing discovery info for {device_address}: {e}")
    
    async def run(self):
        """Main MQTT bridge loop"""
        logger.info(f"Starting MQTT bridge to {self.mqtt_host}:{self.mqtt_port}")
        
        while True:
            try:
                # Connect to MQTT broker
                async with mqtt.Client(
                    hostname=self.mqtt_host,
                    port=self.mqtt_port,
                    username=self.mqtt_username,
                    password=self.mqtt_password
                ) as client:
                    logger.info("Connected to MQTT broker")
                    
                    # Get all devices and publish discovery info
                    devices = self.get_latest_sensor_data()
                    for device_address, data in devices.items():
                        await self.publish_discovery_info(client, device_address, data)
                    
                    # Main publishing loop
                    while True:
                        # Get new data
                        new_data = self.get_new_sensor_data()
                        
                        # Publish new data
                        for device_address, data in new_data.items():
                            await self.publish_device_data(client, device_address, data)
                        
                        # Wait for next cycle
                        await asyncio.sleep(self.publish_interval)
                        
            except Exception as e:
                logger.error(f"MQTT connection error: {e}")
                await asyncio.sleep(30)  # Wait before reconnecting

async def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='MQTT Bridge for Sensor Data')
    parser.add_argument('--db', default='sensor_data.db', help='Database file path')
    parser.add_argument('--mqtt-host', required=True, help='MQTT broker hostname')
    parser.add_argument('--mqtt-port', type=int, default=1883, help='MQTT broker port')
    parser.add_argument('--mqtt-username', help='MQTT username')
    parser.add_argument('--mqtt-password', help='MQTT password')
    parser.add_argument('--topic-prefix', default='sensors', help='MQTT topic prefix')
    parser.add_argument('--publish-interval', type=int, default=60, help='Publish interval in seconds')
    parser.add_argument('--log-level', default='INFO', help='Logging level')
    
    args = parser.parse_args()
    
    # Set log level
    logging.getLogger().setLevel(getattr(logging, args.log_level.upper()))
    
    # Create and run bridge
    bridge = MQTTBridge(
        db_path=args.db,
        mqtt_host=args.mqtt_host,
        mqtt_port=args.mqtt_port,
        mqtt_username=args.mqtt_username,
        mqtt_password=args.mqtt_password,
        topic_prefix=args.topic_prefix,
        publish_interval=args.publish_interval
    )
    
    try:
        await bridge.run()
    except KeyboardInterrupt:
        logger.info("MQTT bridge stopped by user")

if __name__ == "__main__":
    asyncio.run(main())
