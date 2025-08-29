#!/usr/bin/env python3
"""
Sensor Data Viewer - Query and display sensor data from the database
"""

import sqlite3
import argparse
import sys
from datetime import datetime, timedelta
from typing import List, Dict, Any

def query_sensor_data(db_path: str, device_address: str = None, 
                     hours: int = 24, limit: int = 100) -> List[Dict[str, Any]]:
    """Query sensor data from the database"""
    with sqlite3.connect(db_path) as conn:
        cursor = conn.cursor()
        
        # Build query
        query = '''
            SELECT device_address, device_name, timestamp, temperature, 
                   pressure, humidity, battery_mv, power_tier, rssi
            FROM sensor_data
            WHERE timestamp >= datetime('now', '-{} hours')
        '''.format(hours)
        
        params = []
        if device_address:
            query += " AND device_address = ?"
            params.append(device_address)
            
        query += " ORDER BY timestamp DESC LIMIT ?"
        params.append(limit)
        
        cursor.execute(query, params)
        
        # Convert to list of dictionaries
        columns = [desc[0] for desc in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]

def print_sensor_data(data: List[Dict[str, Any]]):
    """Print sensor data in a formatted table"""
    if not data:
        print("No data found")
        return
        
    print(f"\nSensor Data ({len(data)} records):")
    print("-" * 120)
    print(f"{'Device':<18} {'Time':<20} {'Temp(°C)':<8} {'Press(hPa)':<10} {'Hum(%)':<7} {'Batt(mV)':<9} {'Tier':<4} {'RSSI':<5}")
    print("-" * 120)
    
    for record in data:
        timestamp = datetime.fromisoformat(record['timestamp'])
        print(f"{record['device_address']:<18} "
              f"{timestamp.strftime('%Y-%m-%d %H:%M:%S'):<20} "
              f"{record['temperature']:<8.2f} "
              f"{record['pressure']:<10.1f} "
              f"{record['humidity']:<7.2f} "
              f"{record['battery_mv']:<9} "
              f"{record['power_tier']:<4} "
              f"{record['rssi']:<5}")

def print_statistics(data: List[Dict[str, Any]]):
    """Print statistical summary of the data"""
    if not data:
        return
        
    print(f"\nStatistics:")
    print("-" * 50)
    
    # Temperature stats
    temps = [r['temperature'] for r in data if r['temperature'] is not None]
    if temps:
        print(f"Temperature: {min(temps):.2f}°C - {max(temps):.2f}°C (avg: {sum(temps)/len(temps):.2f}°C)")
    
    # Pressure stats
    pressures = [r['pressure'] for r in data if r['pressure'] is not None]
    if pressures:
        print(f"Pressure: {min(pressures):.1f} - {max(pressures):.1f} hPa (avg: {sum(pressures)/len(pressures):.1f} hPa)")
    
    # Humidity stats
    humidities = [r['humidity'] for r in data if r['humidity'] is not None]
    if humidities:
        print(f"Humidity: {min(humidities):.2f} - {max(humidities):.2f}% (avg: {sum(humidities)/len(humidities):.2f}%)")
    
    # Battery stats
    batteries = [r['battery_mv'] for r in data if r['battery_mv'] is not None]
    if batteries:
        print(f"Battery: {min(batteries)} - {max(batteries)} mV (avg: {sum(batteries)/len(batteries):.0f} mV)")
    
    # Power tier distribution
    tiers = {}
    for r in data:
        tier = r['power_tier']
        tiers[tier] = tiers.get(tier, 0) + 1
    
    print(f"Power Tiers: {dict(tiers)}")

def list_devices(db_path: str) -> List[str]:
    """List all unique device addresses in the database"""
    with sqlite3.connect(db_path) as conn:
        cursor = conn.cursor()
        cursor.execute("SELECT DISTINCT device_address FROM sensor_data ORDER BY device_address")
        return [row[0] for row in cursor.fetchall()]

def main():
    parser = argparse.ArgumentParser(description='Sensor Data Viewer')
    parser.add_argument('--db', default='sensor_data.db', help='Database file path')
    parser.add_argument('--device', help='Filter by device address')
    parser.add_argument('--hours', type=int, default=24, help='Hours of data to show')
    parser.add_argument('--limit', type=int, default=100, help='Maximum number of records')
    parser.add_argument('--list-devices', action='store_true', help='List all devices')
    parser.add_argument('--stats', action='store_true', help='Show statistics')
    
    args = parser.parse_args()
    
    try:
        if args.list_devices:
            devices = list_devices(args.db)
            print("Known devices:")
            for device in devices:
                print(f"  {device}")
            return
        
        # Query data
        data = query_sensor_data(args.db, args.device, args.hours, args.limit)
        
        # Display data
        print_sensor_data(data)
        
        # Show statistics if requested
        if args.stats:
            print_statistics(data)
            
    except sqlite3.OperationalError as e:
        print(f"Database error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
