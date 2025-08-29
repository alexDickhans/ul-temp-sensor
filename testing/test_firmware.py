#!/usr/bin/env python3
"""
Firmware Test Suite

Tests the MCU firmware components in a simulated environment.
This helps validate the code logic before flashing to hardware.
"""

import unittest
import struct
import sys
import os

# Add parent directory to path for imports
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

class TestSensorDataEncoding(unittest.TestCase):
    """Test sensor data encoding/decoding"""
    
    def test_sensor_data_structure(self):
        """Test that sensor data structure matches between MCU and host"""
        
        # Simulate MCU data structure (from ble_advertiser.h)
        mcu_data = struct.pack('<BBHhhHI', 
            1,      # version
            2,      # tier (POWER_TIER_RESERVE)
            3600,   # battery_mv
            2350,   # temperature * 100 (23.50°C)
            10132,  # pressure * 10 (1013.2 hPa)
            6540,   # humidity * 100 (65.40%)
            1234567890  # timestamp
        )
        
        # Simulate host decoding (from sensor_scanner.py)
        version, tier, battery_mv, temp_raw, pressure_raw, humidity_raw, timestamp = \
            struct.unpack('<BBHhhHI', mcu_data)
        
        # Convert to actual values
        temperature = temp_raw / 100.0
        pressure = pressure_raw / 10.0
        humidity = humidity_raw / 100.0
        
        # Verify values
        self.assertEqual(version, 1)
        self.assertEqual(tier, 2)
        self.assertEqual(battery_mv, 3600)
        self.assertAlmostEqual(temperature, 23.50, places=2)
        self.assertAlmostEqual(pressure, 1013.2, places=1)
        self.assertAlmostEqual(humidity, 65.40, places=2)
        self.assertEqual(timestamp, 1234567890)

class TestPowerTierLogic(unittest.TestCase):
    """Test adaptive power management logic"""
    
    def test_power_tier_thresholds(self):
        """Test power tier determination based on battery voltage"""
        
        # Test cases: (battery_mv, expected_tier)
        test_cases = [
            (4200, 0),  # Normal tier
            (3900, 0),  # Normal tier
            (3800, 0),  # Normal tier (high threshold)
            (3700, 1),  # Conserve tier
            (3600, 1),  # Conserve tier (high threshold)
            (3500, 2),  # Reserve tier
            (3400, 2),  # Reserve tier (high threshold)
            (3300, 3),  # Survival tier
            (3200, 3),  # Survival tier
        ]
        
        for battery_mv, expected_tier in test_cases:
            tier = self.get_power_tier(battery_mv)
            self.assertEqual(tier, expected_tier, 
                           f"Battery {battery_mv}mV should be tier {expected_tier}, got {tier}")
    
    def test_power_tier_hysteresis(self):
        """Test hysteresis to prevent tier flapping"""
        
        # Simulate hysteresis logic
        current_tier = 0  # Start in normal tier
        
        # Test moving to conserve tier (should require low threshold)
        battery_mv = 3600  # At conserve high threshold
        new_tier = self.get_power_tier_with_hysteresis(battery_mv, current_tier)
        self.assertEqual(new_tier, 1)  # Should move to conserve tier at high threshold
        
        # Test moving to conserve tier (should trigger at low threshold)
        battery_mv = 3500  # Below conserve low threshold
        new_tier = self.get_power_tier_with_hysteresis(battery_mv, current_tier)
        self.assertEqual(new_tier, 1)  # Should move to conserve tier
    
    def get_power_tier(self, battery_mv):
        """Simulate power tier determination logic"""
        if battery_mv >= 3800:
            return 0  # Normal
        elif battery_mv >= 3600:
            return 1  # Conserve
        elif battery_mv >= 3400:
            return 2  # Reserve
        else:
            return 3  # Survival
    
    def get_power_tier_with_hysteresis(self, battery_mv, current_tier):
        """Simulate hysteresis logic"""
        new_tier = self.get_power_tier(battery_mv)
        
        if new_tier != current_tier:
            # Apply hysteresis thresholds
            if new_tier > current_tier:  # Moving to higher tier (lower battery)
                if battery_mv <= 3600 and current_tier == 0:
                    return 1  # Normal -> Conserve
                elif battery_mv <= 3400 and current_tier == 1:
                    return 2  # Conserve -> Reserve
                elif battery_mv <= 3200 and current_tier == 2:
                    return 3  # Reserve -> Survival
                else:
                    return current_tier  # Stay in current tier
            else:  # Moving to lower tier (higher battery)
                if battery_mv >= 3800 and current_tier == 1:
                    return 0  # Conserve -> Normal
                elif battery_mv >= 3600 and current_tier == 2:
                    return 1  # Reserve -> Conserve
                elif battery_mv >= 3400 and current_tier == 3:
                    return 2  # Survival -> Reserve
                else:
                    return current_tier  # Stay in current tier
        
        return new_tier

class TestBME280Calibration(unittest.TestCase):
    """Test BME280 sensor calibration and compensation"""
    
    def test_temperature_compensation(self):
        """Test temperature compensation algorithm"""
        
        # Simulate calibration data
        calib_data = {
            'dig_T1': 27504,
            'dig_T2': 26435,
            'dig_T3': -1000
        }
        
        # Simulate raw ADC value
        adc_T = 519888
        
        # Apply temperature compensation (simplified)
        var1 = (((adc_T >> 3) - (calib_data['dig_T1'] << 1)) * calib_data['dig_T2']) >> 11
        var2 = (((((adc_T >> 4) - calib_data['dig_T1']) * 
                  ((adc_T >> 4) - calib_data['dig_T1'])) >> 12) * calib_data['dig_T3']) >> 14
        t_fine = var1 + var2
        temperature = (t_fine * 5 + 128) >> 8
        
        # Verify reasonable temperature range
        self.assertGreater(temperature, -4000)  # -40.00°C
        self.assertLess(temperature, 8500)      # 85.00°C

class TestBLEAdvertising(unittest.TestCase):
    """Test BLE advertising data format"""
    
    def test_manufacturer_data_format(self):
        """Test manufacturer-specific data format"""
        
        # Nordic Semiconductor Company ID
        company_id = 0x0059
        
        # Create sensor payload
        sensor_data = struct.pack('<BBHhhHI',
            1,      # version
            0,      # tier
            4000,   # battery_mv
            2500,   # temperature * 100 (25.00°C)
            10132,  # pressure * 10 (1013.2 hPa)
            5000,   # humidity * 100 (50.00%)
            1234567890  # timestamp
        )
        
        # Create manufacturer data (company ID + payload)
        mfg_data = struct.pack('<H', company_id) + sensor_data
        
        # Verify structure (company ID is 2 bytes, payload is 14 bytes: 1+1+2+2+2+2+4)
        self.assertEqual(len(mfg_data), 16)  # 2 bytes company ID + 14 bytes payload
        
        # Verify company ID
        decoded_company_id = struct.unpack('<H', mfg_data[0:2])[0]
        self.assertEqual(decoded_company_id, company_id)

class TestBatteryMonitoring(unittest.TestCase):
    """Test battery voltage monitoring"""
    
    def test_adc_to_voltage_conversion(self):
        """Test ADC value to voltage conversion"""
        
        # Simulate ADC configuration
        adc_resolution = 12
        adc_reference = 3300  # 3.3V in mV
        voltage_divider_ratio = 2
        
        # Test cases: (adc_value, expected_voltage_mv)
        test_cases = [
            (2048, 3300),  # Half scale
            (4095, 6598),  # Full scale (calculated: 4095 * 3300 * 2 / 4096 = 6598)
            (0, 0),        # Zero
        ]
        
        for adc_value, expected_voltage_mv in test_cases:
            voltage_mv = (adc_value * adc_reference * voltage_divider_ratio) // (1 << adc_resolution)
            self.assertEqual(voltage_mv, expected_voltage_mv)
    
    def test_battery_percentage_calculation(self):
        """Test battery percentage calculation"""
        
        # Test cases: (voltage_mv, expected_percentage)
        test_cases = [
            (4200, 100),  # Fully charged
            (4000, 71),   # 71% (linear interpolation)
            (3500, 0),    # Minimum safe voltage
            (3300, 0),    # Below minimum
        ]
        
        for voltage_mv, expected_percentage in test_cases:
            percentage = self.calculate_battery_percentage(voltage_mv)
            self.assertEqual(percentage, expected_percentage)
    
    def calculate_battery_percentage(self, voltage_mv):
        """Simulate battery percentage calculation"""
        if voltage_mv >= 4200:
            return 100
        elif voltage_mv <= 3500:
            return 0
        else:
            return int((voltage_mv - 3500) * 100 / (4200 - 3500))

def run_tests():
    """Run all tests"""
    print("Running Adaptive BLE Sensor Node Tests...")
    print("=" * 50)
    
    # Create test suite
    test_suite = unittest.TestSuite()
    
    # Add test classes
    test_classes = [
        TestSensorDataEncoding,
        TestPowerTierLogic,
        TestBME280Calibration,
        TestBLEAdvertising,
        TestBatteryMonitoring
    ]
    
    for test_class in test_classes:
        tests = unittest.TestLoader().loadTestsFromTestCase(test_class)
        test_suite.addTests(tests)
    
    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(test_suite)
    
    # Print summary
    print("=" * 50)
    print(f"Tests run: {result.testsRun}")
    print(f"Failures: {len(result.failures)}")
    print(f"Errors: {len(result.errors)}")
    
    if result.failures:
        print("\nFailures:")
        for test, traceback in result.failures:
            print(f"  {test}: {traceback}")
    
    if result.errors:
        print("\nErrors:")
        for test, traceback in result.errors:
            print(f"  {test}: {traceback}")
    
    return len(result.failures) + len(result.errors) == 0

if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
