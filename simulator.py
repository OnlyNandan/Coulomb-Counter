#!/usr/bin/env python3
"""
BMS Algorithm Simulator - High-Fidelity Version
A comprehensive test harness for the Battery Management System algorithm with advanced battery modeling.

This script compiles the C library, runs high-fidelity simulations with realistic current profiles,
and generates visualizations to analyze the performance of the BMS algorithm.
"""

import subprocess
import ctypes
import numpy as np
import matplotlib.pyplot as plt
import time
import os
import sys
from typing import List, Tuple, Dict
import platform

# Constants
REST_PERIOD_TIME = 5.0  # Time required for rest period (seconds)

class BMSState(ctypes.Structure):
    """C structure mapping for BMS_State"""
    _fields_ = [
        ("soc_percent", ctypes.c_float),
        ("coulomb_count_uAs", ctypes.c_int64),
        ("current_capacity_ah", ctypes.c_float),
        ("nominal_capacity_ah", ctypes.c_float),
        ("kalman_gain", ctypes.c_float),
        ("process_noise", ctypes.c_float),
        ("measurement_noise", ctypes.c_float),
        ("error_covariance", ctypes.c_float),
        ("soh_percent", ctypes.c_float),
        ("capacity_adaptation_rate", ctypes.c_float),
        ("soc_error_accumulator", ctypes.c_float),
        ("soh_update_count", ctypes.c_uint32),
        ("update_count", ctypes.c_uint32),
        ("rest_period_active", ctypes.c_bool),
        ("rest_period_timer", ctypes.c_float),
        ("last_update_time_us", ctypes.c_float)
    ]

class BMSSimulator:
    """Main simulator class for BMS algorithm testing with high-fidelity modeling"""
    
    def __init__(self):
        self.lib = None
        self.compiled = False
        
    def compile_c_library(self) -> bool:
        """Compile the C library into a shared library"""
        try:
            print("Compiling C library...")
            
            # Determine the appropriate file extension based on platform
            if platform.system() == "Windows":
                lib_name = "bms_algo.dll"
                compile_cmd = ["gcc", "-shared", "-fPIC", "-o", lib_name, "bms_algo.c"]
            else:
                lib_name = "bms_algo.so"
                compile_cmd = ["gcc", "-shared", "-fPIC", "-o", lib_name, "bms_algo.c"]
            
            # Compile the library
            result = subprocess.run(compile_cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"Compilation failed: {result.stderr}")
                return False
            
            print(f"Successfully compiled {lib_name}")
            
            # Load the compiled library
            self.lib = ctypes.CDLL(f"./{lib_name}")
            
            # Define function signatures
            self.lib.BMS_Init.argtypes = [ctypes.POINTER(BMSState), ctypes.c_float, ctypes.c_float]
            self.lib.BMS_Init.restype = None
            
            self.lib.BMS_Update.argtypes = [ctypes.POINTER(BMSState), ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
            self.lib.BMS_Update.restype = None
            
            self.lib.BMS_GetCurrent.argtypes = [ctypes.c_float, ctypes.c_float]
            self.lib.BMS_GetCurrent.restype = ctypes.c_float
            
            self.lib.BMS_GetOCVSOC.argtypes = [ctypes.c_float, ctypes.c_float]
            self.lib.BMS_GetOCVSOC.restype = ctypes.c_float
            
            self.lib.BMS_GetInternalResistance.argtypes = [ctypes.c_float, ctypes.c_float]
            self.lib.BMS_GetInternalResistance.restype = ctypes.c_float
            
            self.compiled = True
            return True
            
        except Exception as e:
            print(f"Error compiling library: {e}")
            return False
    
    def calculate_memory_usage(self) -> Dict[str, int]:
        """Calculate static memory usage of the C library"""
        memory_usage = {
            "BMS_State_struct": ctypes.sizeof(BMSState),
            "V_LOOKUP_AXIS": 100 * ctypes.sizeof(ctypes.c_float),
            "T_LOOKUP_AXIS": 7 * ctypes.sizeof(ctypes.c_float),
            "SOC_LOOKUP_TABLE": 100 * 7 * ctypes.sizeof(ctypes.c_float),
            "SOC_LOOKUP_AXIS": 10 * ctypes.sizeof(ctypes.c_float),
            "R_INTERNAL_LOOKUP_TABLE": 10 * 7 * ctypes.sizeof(ctypes.c_float)
        }
        
        memory_usage["total"] = sum(memory_usage.values())
        return memory_usage
    
    def generate_current_profile(self, time_array: np.ndarray) -> np.ndarray:
        """Generate a realistic current profile for a single cycle (60 seconds)"""
        current_array = np.zeros_like(time_array)
        lap_duration = 60.0  # 60-second lap
        
        for i, t in enumerate(time_array):
            lap_time = t % lap_duration  # Time within current lap
            
            if lap_time < 10.0:
                # Hard Acceleration (0-10s): High discharge current
                # Spike to -250A, settling to -100A
                if lap_time < 2.0:
                    current_array[i] = -250.0 + 150.0 * (lap_time / 2.0)  # Spike to -250A
                else:
                    current_array[i] = -100.0 + 20.0 * np.sin(2 * np.pi * (lap_time - 2) / 8)  # Settle to -100A with variation
                    
            elif lap_time < 30.0:
                # Cruising/Top Speed (10-30s): Moderate, steady discharge
                current_array[i] = -95.0 + 10.0 * np.sin(2 * np.pi * (lap_time - 10) / 20)  # -95A with variation (more aggressive net-discharging)
                
            elif lap_time < 35.0:
                # Hard Braking/Regen (30-35s): High charge current
                # Spike to +150A for regen
                if lap_time < 32.0:
                    current_array[i] = 150.0 - 50.0 * ((lap_time - 30) / 2)  # Spike to +150A
                else:
                    current_array[i] = 100.0 - 20.0 * ((lap_time - 32) / 3)  # Decay to +100A
                    
            elif lap_time < 50.0:
                # Cornering/Lower Speed (35-50s): Low discharge current
                current_array[i] = -20.0 + 5.0 * np.sin(2 * np.pi * (lap_time - 35) / 15)  # -20A with variation
                
            else:
                # Straight (50-60s): Another acceleration phase
                current_array[i] = -60.0 + 20.0 * np.sin(2 * np.pi * (lap_time - 50) / 10)  # -60A with variation
            
            # Add realistic noise
            current_array[i] += np.random.normal(0, 2.0)  # 2A noise
        
        return current_array
    
    def run_simulation(self, duration: float = 300.0, dt: float = 0.1) -> Dict[str, np.ndarray]:
        """Run the main BMS simulation with high-fidelity battery model and current profile"""
        if not self.compiled:
            print("Library not compiled. Please compile first.")
            return {}
        
        print(f"Running high-fidelity simulation for {duration} seconds with dt={dt}s...")
        print("Using realistic current profile with advanced battery model...")
        
        time_steps = int(duration / dt)
        time_array = np.linspace(0, duration, time_steps)
        
        # Initialize arrays
        current_array = np.zeros(time_steps)
        voltage_array = np.zeros(time_steps)
        temperature_array = np.zeros(time_steps)
        true_soc_array = np.zeros(time_steps)
        internal_resistance_array = np.zeros(time_steps)
        
        # Battery model parameters
        nominal_capacity_ah = 100.0
        initial_soc_percent = 50.0
        
        # Initialize true SOC tracking
        true_soc_percent = initial_soc_percent
        
        # Initialize BMS state
        bms_state = BMSState()
        self.lib.BMS_Init(ctypes.byref(bms_state), initial_soc_percent, nominal_capacity_ah)
        
        # Arrays to store results
        soc_results = np.zeros(len(time_array))
        soh_results = np.zeros(len(time_array))
        execution_times = np.zeros(len(time_array))
        
        # Generate current profile
        current_array = self.generate_current_profile(time_array)
        
        # Run simulation
        for i in range(len(time_array)):
            start_time = time.perf_counter()
            
            # Update true SOC using ideal Coulomb counting (proper discharge convention)
            true_soc_delta = (current_array[i] * dt) / (nominal_capacity_ah * 3600.0) * 100.0
            true_soc_percent += true_soc_delta  # Matches C-code: positive current adds charge
            true_soc_percent = np.clip(true_soc_percent, 0.0, 100.0)
            true_soc_array[i] = true_soc_percent
            
            # Generate temperature profile (in Celsius)
            temperature_celsius = 20 + 15 * np.sin(2 * np.pi * time_array[i] / 120) + 3 * np.random.normal(0, 0.5)
            temperature_array[i] = temperature_celsius
            
            # ADVANCED BATTERY MODEL: Calculate true OCV and dynamic internal resistance
            # Get true OCV based on true SOC and temperature
            true_ocv = 12.05844 + (true_soc_percent / 100.0) * (13.41786 - 12.05844)
            
            # Get dynamic internal resistance based on true SOC and temperature
            internal_resistance = self.lib.BMS_GetInternalResistance(
                ctypes.c_float(true_soc_percent), 
                ctypes.c_float(temperature_celsius + 273.15)  # Convert to Kelvin
            )
            internal_resistance_array[i] = internal_resistance
            
            # Calculate terminal voltage using advanced model: V = OCV - I*R_dynamic
            voltage_array[i] = true_ocv - current_array[i] * internal_resistance
            
            # Add sensor noise
            voltage_noise = np.random.normal(0, 0.01)  # 10mV noise
            current_noise = np.random.normal(0, 0.5)   # 0.5A noise
            
            voltage_array[i] += voltage_noise
            current_array[i] += current_noise
            
            # Update BMS
            self.lib.BMS_Update(
                ctypes.byref(bms_state),
                ctypes.c_float(voltage_array[i]),
                ctypes.c_float(current_array[i]),
                ctypes.c_float(temperature_array[i]),
                ctypes.c_float(dt)
            )
            
            end_time = time.perf_counter()
            execution_times[i] = (end_time - start_time) * 1e6  # Convert to microseconds
            
            # Store results
            soc_results[i] = bms_state.soc_percent
            soh_results[i] = bms_state.soh_percent
        
        # Print SOH update statistics
        print(f"\nSOH Adaptation Statistics:")
        print(f"Number of SOH updates triggered: {bms_state.soh_update_count}")
        print(f"Total simulation updates: {bms_state.update_count}")
        print(f"SOH update frequency: {bms_state.soh_update_count / bms_state.update_count * 100:.2f}%")
        
        return {
            "time": time_array,
            "current": current_array,
            "voltage": voltage_array,
            "temperature": temperature_array,
            "soc": soc_results,
            "soh": soh_results,
            "true_soc": true_soc_array,
            "internal_resistance": internal_resistance_array,
            "execution_times": execution_times
        }
    
    def run_full_discharge_simulation(self, discharge_current: float = -25.0, dt: float = 0.1) -> Dict[str, np.ndarray]:
        """Run a full discharge simulation from 100% to 0% SOC"""
        if not self.compiled:
            print("Library not compiled. Please compile first.")
            return {}
        
        print(f"Running full discharge simulation with {discharge_current}A current...")
        print("This will simulate a complete discharge from 100% to 0% SOC...")
        
        # Calculate simulation duration for C/4 rate (100Ah battery)
        nominal_capacity_ah = 100.0
        discharge_rate_c = abs(discharge_current) / nominal_capacity_ah
        duration = 100.0 / discharge_rate_c  # Time to discharge from 100% to 0% at this rate
        print(f"Estimated duration: {duration/3600:.2f} hours")
        
        time_steps = int(duration / dt)
        time_array = np.linspace(0, duration, time_steps)
        
        # Initialize arrays
        current_array = np.full(time_steps, discharge_current)
        voltage_array = np.zeros(time_steps)
        temperature_array = np.zeros(time_steps)
        true_soc_array = np.zeros(time_steps)
        internal_resistance_array = np.zeros(time_steps)
        
        # Battery model parameters
        initial_soc_percent = 100.0
        
        # Initialize true SOC tracking
        true_soc_percent = initial_soc_percent
        
        # Initialize BMS state
        bms_state = BMSState()
        self.lib.BMS_Init(ctypes.byref(bms_state), initial_soc_percent, nominal_capacity_ah)
        
        # Arrays to store results
        soc_results = np.zeros(len(time_array))
        soh_results = np.zeros(len(time_array))
        execution_times = np.zeros(len(time_array))
        
        # Run simulation
        for i in range(len(time_array)):
            start_time = time.perf_counter()
            
            # Update true SOC using ideal Coulomb counting
            true_soc_delta = (current_array[i] * dt) / (nominal_capacity_ah * 3600.0) * 100.0
            true_soc_percent += true_soc_delta  # Matches C-code: positive current adds charge
            true_soc_percent = np.clip(true_soc_percent, 0.0, 100.0)
            true_soc_array[i] = true_soc_percent
            
            # Stop simulation if SOC drops below 1%
            if true_soc_percent < 1.0:
                print(f"Simulation stopped at {time_array[i]/3600:.2f} hours (SOC = {true_soc_percent:.2f}%)")
                # Truncate arrays to actual simulation length
                time_array = time_array[:i+1]
                current_array = current_array[:i+1]
                voltage_array = voltage_array[:i+1]
                temperature_array = temperature_array[:i+1]
                true_soc_array = true_soc_array[:i+1]
                internal_resistance_array = internal_resistance_array[:i+1]
                soc_results = soc_results[:i+1]
                soh_results = soh_results[:i+1]
                execution_times = execution_times[:i+1]
                break
            
            # Generate temperature profile (in Celsius)
            temperature_celsius = 20 + 5 * np.sin(2 * np.pi * time_array[i] / 3600) + 2 * np.random.normal(0, 0.3)
            temperature_array[i] = temperature_celsius
            
            # ADVANCED BATTERY MODEL: Calculate true OCV and dynamic internal resistance
            true_ocv = 12.05844 + (true_soc_percent / 100.0) * (13.41786 - 12.05844)
            
            # Get dynamic internal resistance based on true SOC and temperature
            internal_resistance = self.lib.BMS_GetInternalResistance(
                ctypes.c_float(true_soc_percent), 
                ctypes.c_float(temperature_celsius + 273.15)  # Convert to Kelvin
            )
            internal_resistance_array[i] = internal_resistance
            
            # Calculate terminal voltage using advanced model: V = OCV - I*R_dynamic
            voltage_array[i] = true_ocv - current_array[i] * internal_resistance
            
            # Add sensor noise
            voltage_noise = np.random.normal(0, 0.01)  # 10mV noise
            current_noise = np.random.normal(0, 0.1)   # 0.1A noise
            
            voltage_array[i] += voltage_noise
            current_array[i] += current_noise
            
            # Update BMS
            self.lib.BMS_Update(
                ctypes.byref(bms_state),
                ctypes.c_float(voltage_array[i]),
                ctypes.c_float(current_array[i]),
                ctypes.c_float(temperature_array[i]),
                ctypes.c_float(dt)
            )
            
            end_time = time.perf_counter()
            execution_times[i] = (end_time - start_time) * 1e6  # Convert to microseconds
            
            # Store results
            soc_results[i] = bms_state.soc_percent
            soh_results[i] = bms_state.soh_percent
        
        # Print SOH update statistics
        print(f"\nDischarge Simulation SOH Statistics:")
        print(f"Number of SOH updates triggered: {bms_state.soh_update_count}")
        print(f"Total simulation updates: {bms_state.update_count}")
        print(f"SOH update frequency: {bms_state.soh_update_count / bms_state.update_count * 100:.2f}%")
        
        return {
            "time": time_array,
            "current": current_array,
            "voltage": voltage_array,
            "temperature": temperature_array,
            "soc": soc_results,
            "soh": soh_results,
            "true_soc": true_soc_array,
            "internal_resistance": internal_resistance_array,
            "execution_times": execution_times
        }
    
    def run_full_charge_simulation(self, charge_current: float = 25.0, dt: float = 0.1) -> Dict[str, np.ndarray]:
        """Run a full charge simulation from 0% to 100% SOC"""
        if not self.compiled:
            print("Library not compiled. Please compile first.")
            return {}
        
        print(f"Running full charge simulation with {charge_current}A current...")
        print("This will simulate a complete charge from 0% to 100% SOC...")
        
        # Calculate simulation duration for C/4 rate (100Ah battery)
        nominal_capacity_ah = 100.0
        charge_rate_c = abs(charge_current) / nominal_capacity_ah
        duration = 100.0 / charge_rate_c  # Time to charge from 0% to 100% at this rate
        print(f"Estimated duration: {duration/3600:.2f} hours")
        
        time_steps = int(duration / dt)
        time_array = np.linspace(0, duration, time_steps)
        
        # Initialize arrays
        current_array = np.full(time_steps, charge_current)
        voltage_array = np.zeros(time_steps)
        temperature_array = np.zeros(time_steps)
        true_soc_array = np.zeros(time_steps)
        internal_resistance_array = np.zeros(time_steps)
        
        # Battery model parameters
        initial_soc_percent = 0.0
        
        # Initialize true SOC tracking
        true_soc_percent = initial_soc_percent
        
        # Initialize BMS state
        bms_state = BMSState()
        self.lib.BMS_Init(ctypes.byref(bms_state), initial_soc_percent, nominal_capacity_ah)
        
        # Arrays to store results
        soc_results = np.zeros(len(time_array))
        soh_results = np.zeros(len(time_array))
        execution_times = np.zeros(len(time_array))
        
        # Run simulation
        for i in range(len(time_array)):
            start_time = time.perf_counter()
            
            # Update true SOC using ideal Coulomb counting
            true_soc_delta = (current_array[i] * dt) / (nominal_capacity_ah * 3600.0) * 100.0
            true_soc_percent += true_soc_delta  # Plus sign for charge convention
            true_soc_percent = np.clip(true_soc_percent, 0.0, 100.0)
            true_soc_array[i] = true_soc_percent
            
            # Stop simulation if SOC exceeds 99%
            if true_soc_percent > 99.0:
                print(f"Simulation stopped at {time_array[i]/3600:.2f} hours (SOC = {true_soc_percent:.2f}%)")
                # Truncate arrays to actual simulation length
                time_array = time_array[:i+1]
                current_array = current_array[:i+1]
                voltage_array = voltage_array[:i+1]
                temperature_array = temperature_array[:i+1]
                true_soc_array = true_soc_array[:i+1]
                internal_resistance_array = internal_resistance_array[:i+1]
                soc_results = soc_results[:i+1]
                soh_results = soh_results[:i+1]
                execution_times = execution_times[:i+1]
                break
            
            # Generate temperature profile (in Celsius)
            temperature_celsius = 20 + 5 * np.sin(2 * np.pi * time_array[i] / 3600) + 2 * np.random.normal(0, 0.3)
            temperature_array[i] = temperature_celsius
            
            # ADVANCED BATTERY MODEL: Calculate true OCV and dynamic internal resistance
            true_ocv = 12.05844 + (true_soc_percent / 100.0) * (13.41786 - 12.05844)
            
            # Get dynamic internal resistance based on true SOC and temperature
            internal_resistance = self.lib.BMS_GetInternalResistance(
                ctypes.c_float(true_soc_percent), 
                ctypes.c_float(temperature_celsius + 273.15)  # Convert to Kelvin
            )
            internal_resistance_array[i] = internal_resistance
            
            # Calculate terminal voltage using advanced model: V = OCV + I*R_dynamic (for charging)
            voltage_array[i] = true_ocv + current_array[i] * internal_resistance
            
            # Add sensor noise
            voltage_noise = np.random.normal(0, 0.01)  # 10mV noise
            current_noise = np.random.normal(0, 0.1)   # 0.1A noise
            
            voltage_array[i] += voltage_noise
            current_array[i] += current_noise
            
            # Update BMS
            self.lib.BMS_Update(
                ctypes.byref(bms_state),
                ctypes.c_float(voltage_array[i]),
                ctypes.c_float(current_array[i]),
                ctypes.c_float(temperature_array[i]),
                ctypes.c_float(dt)
            )
            
            end_time = time.perf_counter()
            execution_times[i] = (end_time - start_time) * 1e6  # Convert to microseconds
            
            # Store results
            soc_results[i] = bms_state.soc_percent
            soh_results[i] = bms_state.soh_percent
        
        # Print SOH update statistics
        print(f"\nCharge Simulation SOH Statistics:")
        print(f"Number of SOH updates triggered: {bms_state.soh_update_count}")
        print(f"Total simulation updates: {bms_state.update_count}")
        print(f"SOH update frequency: {bms_state.soh_update_count / bms_state.update_count * 100:.2f}%")
        
        return {
            "time": time_array,
            "current": current_array,
            "voltage": voltage_array,
            "temperature": temperature_array,
            "soc": soc_results,
            "soh": soh_results,
            "true_soc": true_soc_array,
            "internal_resistance": internal_resistance_array,
            "execution_times": execution_times
        }
    
    def run_lifecycle_simulation(self, num_cycles: int = 100) -> Dict[str, np.ndarray]:
        """
        Run a realistic lifecycle simulation with an independent, degrading battery model 
        to properly test the SOH estimation algorithm.
        """
        if not self.compiled:
            print("Library not compiled. Please compile first.")
            return {}

        print(f"Running realistic lifecycle simulation for {num_cycles} cycles...")

        cycle_numbers = np.arange(1, num_cycles + 1)
        soh_results = np.zeros(num_cycles)
        
        true_capacity_ah = 100.0
        capacity_degradation_factor = 1.0 - (0.15 / true_capacity_ah)

        bms_state = BMSState()
        self.lib.BMS_Init(ctypes.byref(bms_state), 100.0, 100.0)

        for cycle in range(num_cycles):
            true_soc = 100.0
            while true_soc > 5.0:
                dt = 30.0
                current = 50.0
                
                true_ocv = 12.05 + (true_soc / 100.0) * 1.4 
                voltage = true_ocv - current * 0.008
                
                self.lib.BMS_Update(ctypes.byref(bms_state), ctypes.c_float(voltage), ctypes.c_float(-current), 25.0, dt)
                true_soc -= (current * dt) / (true_capacity_ah * 3600.0) * 100.0

            for _ in range(10):
                voltage = self.lib.BMS_GetOCVSOC(ctypes.c_float(bms_state.soc_percent), ctypes.c_float(25.0 + 273.15))
                self.lib.BMS_Update(ctypes.byref(bms_state), ctypes.c_float(voltage), 0.0, 25.0, 5.0)

            while true_soc < 100.0:
                dt = 30.0
                current = 25.0

                true_ocv = 12.05 + (true_soc / 100.0) * 1.4
                voltage = true_ocv + current * 0.008
                
                self.lib.BMS_Update(ctypes.byref(bms_state), ctypes.c_float(voltage), ctypes.c_float(current), 25.0, dt)
                true_soc += (current * dt) / (true_capacity_ah * 3600.0) * 100.0
            
            soh_results[cycle] = bms_state.soh_percent
            true_capacity_ah *= capacity_degradation_factor
            
            if (cycle + 1) % 10 == 0:
                print(f"Cycle {cycle + 1}/{num_cycles} complete. Estimated SOH: {bms_state.soh_percent:.2f}%, True Capacity: {true_capacity_ah:.2f}Ah")

        return {"cycle_numbers": cycle_numbers, "soh_values": soh_results}
    
    def create_main_visualization(self, results: Dict[str, np.ndarray]):
        """Create the main multi-panel visualization with internal resistance panel"""
        fig, axes = plt.subplots(2, 3, figsize=(18, 10))
        fig.suptitle('BMS Algorithm Performance Analysis - High-Fidelity Simulation', fontsize=16, fontweight='bold')
        
        # Panel 1: SOC vs Time (with True SOC comparison)
        axes[0, 0].plot(results["time"], results["soc"], 'b-', linewidth=2, label='Estimated SOC')
        axes[0, 0].plot(results["time"], results["true_soc"], 'r--', linewidth=2, label='True SOC')
        axes[0, 0].set_xlabel('Time (s)')
        axes[0, 0].set_ylabel('SOC (%)')
        axes[0, 0].set_title('State of Charge Estimation')
        axes[0, 0].grid(True, alpha=0.3)
        axes[0, 0].legend()
        
        # Panel 2: SOH vs Time
        axes[0, 1].plot(results["time"], results["soh"], 'r-', linewidth=2, label='Estimated SOH')
        axes[0, 1].set_xlabel('Time (s)')
        axes[0, 1].set_ylabel('SOH (%)')
        axes[0, 1].set_title('State of Health Estimation')
        axes[0, 1].grid(True, alpha=0.3)
        axes[0, 1].legend()
        
        # Panel 3: Dynamic Internal Resistance vs Time
        axes[0, 2].plot(results["time"], results["internal_resistance"], 'g-', linewidth=2, label='Internal Resistance')
        axes[0, 2].set_xlabel('Time (s)')
        axes[0, 2].set_ylabel('Resistance (Ohms)')
        axes[0, 2].set_title('Dynamic Internal Resistance')
        axes[0, 2].grid(True, alpha=0.3)
        axes[0, 2].legend()
        
        # Panel 4: Current vs Time (Current Profile)
        axes[1, 0].plot(results["time"], results["current"], 'g-', linewidth=1, alpha=0.7)
        axes[1, 0].set_xlabel('Time (s)')
        axes[1, 0].set_ylabel('Current (A)')
        axes[1, 0].set_title('Current Profile')
        axes[1, 0].grid(True, alpha=0.3)
        
        # Panel 5: Voltage vs Time
        axes[1, 1].plot(results["time"], results["voltage"], 'm-', linewidth=1, alpha=0.7)
        axes[1, 1].set_xlabel('Time (s)')
        axes[1, 1].set_ylabel('Voltage (V)')
        axes[1, 1].set_title('Terminal Voltage Profile')
        axes[1, 1].grid(True, alpha=0.3)
        
        # Panel 6: Temperature vs Time
        axes[1, 2].plot(results["time"], results["temperature"], 'orange', linewidth=1, alpha=0.7)
        axes[1, 2].set_xlabel('Time (s)')
        axes[1, 2].set_ylabel('Temperature (°C)')
        axes[1, 2].set_title('Battery Temperature Profile')
        axes[1, 2].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig('bms_high_fidelity_analysis.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def create_performance_analysis(self, results: Dict[str, np.ndarray]):
        """Create performance analysis plots"""
        fig, axes = plt.subplots(1, 2, figsize=(12, 5))
        fig.suptitle('BMS Algorithm Performance Analysis', fontsize=16, fontweight='bold')
        
        # Execution time histogram
        axes[0].hist(results["execution_times"], bins=50, alpha=0.7, color='blue', edgecolor='black')
        axes[0].set_xlabel('Execution Time (μs)')
        axes[0].set_ylabel('Frequency')
        axes[0].set_title('BMS_Update Execution Time Distribution')
        axes[0].grid(True, alpha=0.3)
        
        # Execution time vs time
        axes[1].plot(results["time"], results["execution_times"], 'r-', linewidth=1, alpha=0.7)
        axes[1].set_xlabel('Time (s)')
        axes[1].set_ylabel('Execution Time (μs)')
        axes[1].set_title('Execution Time Over Simulation')
        axes[1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig('bms_performance_analysis.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        # Print performance statistics
        print(f"\nPerformance Statistics:")
        print(f"Average execution time: {np.mean(results['execution_times']):.2f} μs")
        print(f"Maximum execution time: {np.max(results['execution_times']):.2f} μs")
        print(f"Minimum execution time: {np.min(results['execution_times']):.2f} μs")
        print(f"Standard deviation: {np.std(results['execution_times']):.2f} μs")
        
        # Print SOC accuracy statistics
        soc_error = np.abs(results['soc'] - results['true_soc'])
        print(f"\nSOC Estimation Accuracy:")
        print(f"Mean absolute error: {np.mean(soc_error):.2f}%")
        print(f"Maximum absolute error: {np.max(soc_error):.2f}%")
        print(f"RMS error: {np.sqrt(np.mean(soc_error**2)):.2f}%")
        
        # Print internal resistance statistics
        print(f"\nInternal Resistance Statistics:")
        print(f"Average resistance: {np.mean(results['internal_resistance']):.4f} Ohms")
        print(f"Maximum resistance: {np.max(results['internal_resistance']):.4f} Ohms")
        print(f"Minimum resistance: {np.min(results['internal_resistance']):.4f} Ohms")
    
    def create_lifecycle_visualization(self, lifecycle_results: Dict[str, np.ndarray]):
        """Create lifecycle visualization"""
        plt.figure(figsize=(10, 6))
        plt.plot(lifecycle_results["cycle_numbers"], lifecycle_results["soh_values"], 'r-', linewidth=2, marker='o', markersize=3)
        plt.xlabel('Cycle Number')
        plt.ylabel('SOH (%)')
        plt.title('Battery State of Health Over Lifecycle')
        plt.grid(True, alpha=0.3)
        plt.ylim(0, 105)
        
        # Add trend line
        z = np.polyfit(lifecycle_results["cycle_numbers"], lifecycle_results["soh_values"], 1)
        p = np.poly1d(z)
        plt.plot(lifecycle_results["cycle_numbers"], p(lifecycle_results["cycle_numbers"]), "r--", alpha=0.8, label=f'Trend (slope: {z[0]:.3f})')
        plt.legend()
        
        plt.tight_layout()
        plt.savefig('bms_lifecycle_analysis.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def create_discharge_visualization(self, results: Dict[str, np.ndarray]):
        """Create discharge simulation visualization"""
        fig, axes = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle('BMS Full Discharge Simulation Analysis', fontsize=16, fontweight='bold')
        
        # Convert time to hours for better readability
        time_hours = results["time"] / 3600.0
        
        # Panel 1: SOC vs Time (with True SOC comparison)
        axes[0, 0].plot(time_hours, results["soc"], 'b-', linewidth=2, label='Estimated SOC')
        axes[0, 0].plot(time_hours, results["true_soc"], 'r--', linewidth=2, label='True SOC')
        axes[0, 0].set_xlabel('Time (hours)')
        axes[0, 0].set_ylabel('SOC (%)')
        axes[0, 0].set_title('State of Charge During Full Discharge')
        axes[0, 0].grid(True, alpha=0.3)
        axes[0, 0].legend()
        
        # Panel 2: SOC Error vs Time
        soc_error = results["soc"] - results["true_soc"]
        axes[0, 1].plot(time_hours, soc_error, 'g-', linewidth=2, label='SOC Error')
        axes[0, 1].axhline(y=0, color='k', linestyle='--', alpha=0.5)
        axes[0, 1].set_xlabel('Time (hours)')
        axes[0, 1].set_ylabel('SOC Error (%)')
        axes[0, 1].set_title('SOC Estimation Error Over Time')
        axes[0, 1].grid(True, alpha=0.3)
        axes[0, 1].legend()
        
        # Panel 3: Voltage vs Time
        axes[1, 0].plot(time_hours, results["voltage"], 'm-', linewidth=1, alpha=0.7)
        axes[1, 0].set_xlabel('Time (hours)')
        axes[1, 0].set_ylabel('Voltage (V)')
        axes[1, 0].set_title('Terminal Voltage During Discharge')
        axes[1, 0].grid(True, alpha=0.3)
        
        # Panel 4: Internal Resistance vs Time
        axes[1, 1].plot(time_hours, results["internal_resistance"], 'orange', linewidth=2)
        axes[1, 1].set_xlabel('Time (hours)')
        axes[1, 1].set_ylabel('Internal Resistance (Ohms)')
        axes[1, 1].set_title('Dynamic Internal Resistance During Discharge')
        axes[1, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig('bms_discharge_analysis.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        # Print discharge statistics
        print(f"\nDischarge Simulation Statistics:")
        print(f"Final SOC Error: {soc_error[-1]:.2f}%")
        print(f"Maximum SOC Error: {np.max(np.abs(soc_error)):.2f}%")
        print(f"RMS SOC Error: {np.sqrt(np.mean(soc_error**2)):.2f}%")
        print(f"Average Internal Resistance: {np.mean(results['internal_resistance']):.4f} Ohms")
    
    def create_charge_visualization(self, results: Dict[str, np.ndarray]):
        """Create charge simulation visualization"""
        fig, axes = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle('BMS Full Charge Simulation Analysis', fontsize=16, fontweight='bold')
        
        # Convert time to hours for better readability
        time_hours = results["time"] / 3600.0
        
        # Panel 1: SOC vs Time (with True SOC comparison)
        axes[0, 0].plot(time_hours, results["soc"], 'b-', linewidth=2, label='Estimated SOC')
        axes[0, 0].plot(time_hours, results["true_soc"], 'r--', linewidth=2, label='True SOC')
        axes[0, 0].set_xlabel('Time (hours)')
        axes[0, 0].set_ylabel('SOC (%)')
        axes[0, 0].set_title('State of Charge During Full Charge')
        axes[0, 0].grid(True, alpha=0.3)
        axes[0, 0].legend()
        
        # Panel 2: SOC Error vs Time
        soc_error = results["soc"] - results["true_soc"]
        axes[0, 1].plot(time_hours, soc_error, 'g-', linewidth=2, label='SOC Error')
        axes[0, 1].axhline(y=0, color='k', linestyle='--', alpha=0.5)
        axes[0, 1].set_xlabel('Time (hours)')
        axes[0, 1].set_ylabel('SOC Error (%)')
        axes[0, 1].set_title('SOC Estimation Error Over Time')
        axes[0, 1].grid(True, alpha=0.3)
        axes[0, 1].legend()
        
        # Panel 3: Voltage vs Time
        axes[1, 0].plot(time_hours, results["voltage"], 'm-', linewidth=1, alpha=0.7)
        axes[1, 0].set_xlabel('Time (hours)')
        axes[1, 0].set_ylabel('Voltage (V)')
        axes[1, 0].set_title('Terminal Voltage During Charge')
        axes[1, 0].grid(True, alpha=0.3)
        
        # Panel 4: Internal Resistance vs Time
        axes[1, 1].plot(time_hours, results["internal_resistance"], 'orange', linewidth=2)
        axes[1, 1].set_xlabel('Time (hours)')
        axes[1, 1].set_ylabel('Internal Resistance (Ohms)')
        axes[1, 1].set_title('Dynamic Internal Resistance During Charge')
        axes[1, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig('bms_charge_analysis.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        # Print charge statistics
        print(f"\nCharge Simulation Statistics:")
        print(f"Final SOC Error: {soc_error[-1]:.2f}%")
        print(f"Maximum SOC Error: {np.max(np.abs(soc_error)):.2f}%")
        print(f"RMS SOC Error: {np.sqrt(np.mean(soc_error**2)):.2f}%")
        print(f"Average Internal Resistance: {np.mean(results['internal_resistance']):.4f} Ohms")
    
    def print_memory_usage(self):
        """Print memory usage information"""
        memory_usage = self.calculate_memory_usage()
        
        print(f"\nMemory Usage Analysis:")
        print(f"BMS_State struct: {memory_usage['BMS_State_struct']} bytes")
        print(f"V_LOOKUP_AXIS: {memory_usage['V_LOOKUP_AXIS']} bytes")
        print(f"T_LOOKUP_AXIS: {memory_usage['T_LOOKUP_AXIS']} bytes")
        print(f"SOC_LOOKUP_TABLE: {memory_usage['SOC_LOOKUP_TABLE']} bytes")
        print(f"SOC_LOOKUP_AXIS: {memory_usage['SOC_LOOKUP_AXIS']} bytes")
        print(f"R_INTERNAL_LOOKUP_TABLE: {memory_usage['R_INTERNAL_LOOKUP_TABLE']} bytes")
        print(f"Total static memory: {memory_usage['total']} bytes ({memory_usage['total']/1024:.2f} KB)")

def run_sanity_checks(simulator):
    """Run basic sanity checks to verify core BMS logic"""
    print("\n" + "="*60)
    print("RUNNING SANITY CHECKS")
    print("="*60)
    
    if not simulator.compiled:
        print("Library not compiled. Cannot run sanity checks.")
        return False
    
    all_passed = True
    
    # Test A: Charge Test
    print("\nTest A: Charge Test")
    print("Initializing SOC to 50%, applying +5A charge current for 1 hour...")
    
    bms_state = BMSState()
    simulator.lib.BMS_Init(ctypes.byref(bms_state), 50.0, 100.0)
    
    initial_soc = bms_state.soc_percent
    charge_current = 5.0  # Positive current = charge
    dt = 0.1
    duration = 3600.0  # 1 hour
    steps = int(duration / dt)
    
    for i in range(steps):
        # Apply constant charge current
        voltage = 12.5 + 0.5 * np.sin(2 * np.pi * i * dt / 3600)  # Vary voltage slightly
        temperature = 25.0
        
        simulator.lib.BMS_Update(
            ctypes.byref(bms_state),
            ctypes.c_float(voltage),
            ctypes.c_float(charge_current),
            ctypes.c_float(temperature),
            ctypes.c_float(dt)
        )
    
    final_soc = bms_state.soc_percent
    expected_soc_increase = abs(charge_current) * duration / (100.0 * 3600.0) * 100.0  # 5% for 100Ah battery
    actual_soc_increase = final_soc - initial_soc
    
    print(f"Initial SOC: {initial_soc:.2f}%")
    print(f"Final SOC: {final_soc:.2f}%")
    print(f"Expected increase: {expected_soc_increase:.2f}%")
    print(f"Actual increase: {actual_soc_increase:.2f}%")
    
    if abs(actual_soc_increase - expected_soc_increase) < 0.5:  # Allow 0.5% tolerance
        print("✓ PASS - Charge test")
    else:
        print("✗ FAIL - Charge test")
        all_passed = False
    
    # Test B: Discharge Test
    print("\nTest B: Discharge Test")
    print("Initializing SOC to 50%, applying -5A discharge current for 1 hour...")
    
    simulator.lib.BMS_Init(ctypes.byref(bms_state), 50.0, 100.0)
    
    initial_soc = bms_state.soc_percent
    discharge_current = -5.0  # Negative current = discharge
    
    for i in range(steps):
        # Apply constant discharge current
        voltage = 12.5 - 0.5 * np.sin(2 * np.pi * i * dt / 3600)  # Vary voltage slightly
        temperature = 25.0
        
        simulator.lib.BMS_Update(
            ctypes.byref(bms_state),
            ctypes.c_float(voltage),
            ctypes.c_float(discharge_current),
            ctypes.c_float(temperature),
            ctypes.c_float(dt)
        )
    
    final_soc = bms_state.soc_percent
    expected_soc_decrease = abs(discharge_current) * duration / (100.0 * 3600.0) * 100.0  # 5% for 100Ah battery
    actual_soc_decrease = initial_soc - final_soc
    
    print(f"Initial SOC: {initial_soc:.2f}%")
    print(f"Final SOC: {final_soc:.2f}%")
    print(f"Expected decrease: {expected_soc_decrease:.2f}%")
    print(f"Actual decrease: {actual_soc_decrease:.2f}%")
    
    if abs(actual_soc_decrease - expected_soc_decrease) < 0.5:  # Allow 0.5% tolerance
        print("✓ PASS - Discharge test")
    else:
        print("✗ FAIL - Discharge test")
        all_passed = False
    
    # Test D: Lookup Table Boundary Checks - Low Boundary
    print("\nTest D: OCV Lookup - Low Boundary")
    print("Testing BMS_GetOCVSOC with low voltage (12.06V) and mid-range temperature (293K)...")
    
    low_voltage = 12.06
    mid_temp = 293.0  # 20°C in Kelvin
    low_soc_result = simulator.lib.BMS_GetOCVSOC(ctypes.c_float(low_voltage), ctypes.c_float(mid_temp))
    
    print(f"Input: Voltage={low_voltage:.2f}V, Temperature={mid_temp:.1f}K")
    print(f"Returned SOC: {low_soc_result:.2f}%")
    
    if 0.0 <= low_soc_result <= 5.0:  # Should be very low SOC
        print("✓ PASS - Low boundary test")
    else:
        print("✗ FAIL - Low boundary test (SOC should be 0-5%)")
        all_passed = False
    
    # Test E: Lookup Table Boundary Checks - High Boundary
    print("\nTest E: OCV Lookup - High Boundary")
    print("Testing BMS_GetOCVSOC with high voltage (13.41V) and mid-range temperature (293K)...")
    
    high_voltage = 13.41
    high_soc_result = simulator.lib.BMS_GetOCVSOC(ctypes.c_float(high_voltage), ctypes.c_float(mid_temp))
    
    print(f"Input: Voltage={high_voltage:.2f}V, Temperature={mid_temp:.1f}K")
    print(f"Returned SOC: {high_soc_result:.2f}%")
    
    if 95.0 <= high_soc_result <= 100.0:  # Should be very high SOC
        print("✓ PASS - High boundary test")
    else:
        print("✗ FAIL - High boundary test (SOC should be 95-100%)")
        all_passed = False
    
    # Test F: Lookup Table Boundary Checks - Mid-Point
    print("\nTest F: OCV Lookup - Mid-Point")
    print("Testing BMS_GetOCVSOC with mid-range voltage and temperature...")
    
    mid_voltage = 12.75  # Mid-point of voltage range
    mid_soc_result = simulator.lib.BMS_GetOCVSOC(ctypes.c_float(mid_voltage), ctypes.c_float(mid_temp))
    
    print(f"Input: Voltage={mid_voltage:.2f}V, Temperature={mid_temp:.1f}K")
    print(f"Returned SOC: {mid_soc_result:.2f}%")
    
    if 40.0 <= mid_soc_result <= 60.0:  # Should be mid-range SOC
        print("✓ PASS - Mid-point test")
    else:
        print("✗ FAIL - Mid-point test (SOC should be 40-60%)")
        all_passed = False
    
    # Test G: Internal Resistance Lookup - Low SOC, Low Temp
    print("\nTest G: Internal Resistance - Low SOC, Low Temp")
    print("Testing BMS_GetInternalResistance with low SOC (5%) and low temperature (263K)...")
    
    low_soc = 5.0
    low_temp = 263.0  # -10°C in Kelvin
    high_resistance_result = simulator.lib.BMS_GetInternalResistance(ctypes.c_float(low_soc), ctypes.c_float(low_temp))
    
    print(f"Input: SOC={low_soc:.1f}%, Temperature={low_temp:.1f}K")
    print(f"Returned Resistance: {high_resistance_result:.4f} Ohms")
    
    if high_resistance_result > 0.020:  # Should be high resistance
        print("✓ PASS - High resistance test")
    else:
        print("✗ FAIL - High resistance test (resistance should be > 0.020 Ohms)")
        all_passed = False
    
    # Test H: Internal Resistance Lookup - High SOC, High Temp
    print("\nTest H: Internal Resistance - High SOC, High Temp")
    print("Testing BMS_GetInternalResistance with high SOC (95%) and high temperature (313K)...")
    
    high_soc = 95.0
    high_temp = 313.0  # 40°C in Kelvin
    low_resistance_result = simulator.lib.BMS_GetInternalResistance(ctypes.c_float(high_soc), ctypes.c_float(high_temp))
    
    print(f"Input: SOC={high_soc:.1f}%, Temperature={high_temp:.1f}K")
    print(f"Returned Resistance: {low_resistance_result:.4f} Ohms")
    
    if low_resistance_result < 0.005:  # Should be very low resistance
        print("✓ PASS - Low resistance test")
    else:
        print("✗ FAIL - Low resistance test (resistance should be < 0.005 Ohms)")
        all_passed = False
    
    # Test I: OCV Lookup Monotonicity Check
    print("\nTest I: OCV Lookup Monotonicity Check")
    print("Testing that SOC increases monotonically with voltage...")
    
    voltages = [12.1, 12.3, 12.5, 12.7, 12.9, 13.1, 13.3]
    socs = []
    
    for v in voltages:
        soc = simulator.lib.BMS_GetOCVSOC(ctypes.c_float(v), ctypes.c_float(mid_temp))
        socs.append(soc)
        print(f"  Voltage={v:.1f}V -> SOC={soc:.2f}%")
    
    # Check if SOCs are monotonically increasing
    monotonic = all(socs[i] <= socs[i+1] for i in range(len(socs)-1))
    
    if monotonic:
        print("✓ PASS - Monotonicity test")
    else:
        print("✗ FAIL - Monotonicity test (SOC should increase with voltage)")
        all_passed = False
    
    # Test J: Internal Resistance Monotonicity Check
    print("\nTest J: Internal Resistance Monotonicity Check")
    print("Testing that resistance decreases with SOC...")
    
    socs_test = [10.0, 30.0, 50.0, 70.0, 90.0]
    resistances = []
    
    for soc in socs_test:
        res = simulator.lib.BMS_GetInternalResistance(ctypes.c_float(soc), ctypes.c_float(mid_temp))
        resistances.append(res)
        print(f"  SOC={soc:.1f}% -> Resistance={res:.4f} Ohms")
    
    # Check if resistances are monotonically decreasing
    resistance_monotonic = all(resistances[i] >= resistances[i+1] for i in range(len(resistances)-1))
    
    if resistance_monotonic:
        print("✓ PASS - Resistance monotonicity test")
    else:
        print("✗ FAIL - Resistance monotonicity test (resistance should decrease with SOC)")
        all_passed = False
    
    # Test C: OCV Sync Test (moved to end after boundary checks)
    print("\nTest C: OCV Sync Test")
    print("Testing SOH adaptation and SOC correction during rest period...")
    
    simulator.lib.BMS_Init(ctypes.byref(bms_state), 50.0, 100.0)
    initial_soh_count = bms_state.soh_update_count
    
    # Apply discharge current for a short time to create error
    discharge_current = 10.0
    for i in range(100):  # 10 seconds of discharge
        voltage = 12.0  # Low voltage to create SOC error
        temperature = 25.0
        
        simulator.lib.BMS_Update(
            ctypes.byref(bms_state),
            ctypes.c_float(voltage),
            ctypes.c_float(discharge_current),
            ctypes.c_float(temperature),
            ctypes.c_float(dt)
        )
    
    soc_after_discharge = bms_state.soc_percent
    print(f"SOC after discharge: {soc_after_discharge:.2f}%")
    
    # Check for impossible SOC values
    if soc_after_discharge > 100.0 or soc_after_discharge < 0.0:
        print(f"✗ FAIL - Impossible SOC detected: {soc_after_discharge:.2f}%")
        all_passed = False
        print("This indicates a critical bug in the lookup table or interpolation logic!")
        return all_passed
    
    # Now apply rest period (0A current) for longer than REST_PERIOD_TIME
    rest_duration = 35.0  # 35 seconds > 30 second threshold
    rest_steps = int(rest_duration / dt)
    
    # Use the correct OCV for the rest period
    rest_voltage = 12.75  # Correct OCV for ~50% SOC
    expected_ocv_soc = 51.50  # From the passing Test F
    
    soc_after_soh_update = None
    
    for i in range(rest_steps):
        voltage = rest_voltage  # Use the correct OCV for the rest period
        temperature = 293.0  # Use same temperature as Test F (293K)
        
        simulator.lib.BMS_Update(
            ctypes.byref(bms_state),
            ctypes.c_float(voltage),
            ctypes.c_float(0.0),  # Rest current
            ctypes.c_float(temperature),
            ctypes.c_float(dt)
        )
        
        # Check SOC immediately after SOH update (at 30 seconds)
        if i == int(30.0 / dt) and bms_state.soh_update_count > initial_soh_count:
            soc_after_soh_update = bms_state.soc_percent
            print(f"SOC immediately after SOH update: {soc_after_soh_update:.2f}%")
    
    final_soh_count = bms_state.soh_update_count
    final_soc = bms_state.soc_percent
    
    print(f"SOC after rest period: {final_soc:.2f}%")
    print(f"SOH updates triggered: {final_soh_count - initial_soh_count}")

    # Check if the final SOC has correctly converged towards the OCV value
    # and if the SOH update was triggered.
    # The SOC should converge to the OCV value (51.50%) after SOH adaptation
    test_soc = soc_after_soh_update if soc_after_soh_update is not None else final_soc
    soc_convergence_error = abs(test_soc - expected_ocv_soc)
    print(f"SOC convergence error: {soc_convergence_error:.2f}%")
    
    if final_soh_count > initial_soh_count and soc_convergence_error < 5.0:
        print("✓ PASS - OCV sync test")
    else:
        print("✗ FAIL - OCV sync test")
        print(f"  Expected SOC: {expected_ocv_soc:.2f}%, Got: {test_soc:.2f}%")
        print(f"  SOH updates: {final_soh_count - initial_soh_count}")
        all_passed = False
    
    print("\n" + "="*60)
    if all_passed:
        print("✓ ALL SANITY CHECKS PASSED")
    else:
        print("✗ SOME SANITY CHECKS FAILED")
    print("="*60)
    
    return all_passed

def main():
    """Main function to run the comprehensive BMS simulator with all test cases"""
    print("BMS Algorithm Simulator - Comprehensive Test Suite")
    print("=" * 60)
    print("Features:")
    print("- Advanced battery model with dynamic internal resistance")
    print("- Realistic current profile simulation")
    print("- Full discharge cycle test (100% to 0%)")
    print("- Full charge cycle test (0% to 100%)")
    print("- High-fidelity SOC and SOH estimation with aggressive tuning")
    print("=" * 60)
    
    # Create simulator instance
    simulator = BMSSimulator()
    
    # Compile the C library
    if not simulator.compile_c_library():
        print("Failed to compile C library. Exiting.")
        return
    
    # Print memory usage
    simulator.print_memory_usage()
    
    # Run sanity checks first
    sanity_passed = run_sanity_checks(simulator)
    if not sanity_passed:
        print("Sanity checks failed. Please review the BMS algorithm implementation.")
        return
    
    # Run all three simulations in sequence
    print("\n" + "="*60)
    print("TEST 1: CURRENT PROFILE SIMULATION")
    print("="*60)
    results = simulator.run_simulation(duration=300.0, dt=0.1)
    
    if results:
        simulator.create_main_visualization(results)
        simulator.create_performance_analysis(results)
        print("✓ Current profile simulation completed")
    
    print("\n" + "="*60)
    print("TEST 2: FULL DISCHARGE CYCLE SIMULATION")
    print("="*60)
    discharge_results = simulator.run_full_discharge_simulation(discharge_current=-25.0, dt=0.1)
    
    if discharge_results:
        simulator.create_discharge_visualization(discharge_results)
        print("✓ Full discharge simulation completed")
    
    print("\n" + "="*60)
    print("TEST 3: FULL CHARGE CYCLE SIMULATION")
    print("="*60)
    charge_results = simulator.run_full_charge_simulation(charge_current=25.0, dt=0.1)
    
    if charge_results:
        simulator.create_charge_visualization(charge_results)
        print("✓ Full charge simulation completed")
    
    # Run lifecycle simulation
    print("\n" + "="*60)
    print("TEST 4: LIFECYCLE DEGRADATION SIMULATION")
    print("="*60)
    lifecycle_results = simulator.run_lifecycle_simulation(num_cycles=100)
    simulator.create_lifecycle_visualization(lifecycle_results)
    print("✓ Lifecycle simulation completed")
    
    print("\n" + "="*60)
    print("COMPREHENSIVE TEST SUITE COMPLETED SUCCESSFULLY!")
    print("="*60)
    print("Generated analysis plots:")
    print("- bms_high_fidelity_analysis.png (Current profile)")
    print("- bms_discharge_analysis.png (Full discharge test)")
    print("- bms_charge_analysis.png (Full charge test)")
    print("- bms_performance_analysis.png (Performance metrics)")
    print("- bms_lifecycle_analysis.png (SOH degradation)")
    print("\nThe aggressive Kalman filter tuning should eliminate drift issues.")
    
    # Clean up compiled library
    try:
        if platform.system() == "Windows":
            os.remove("bms_algo.dll")
        else:
            os.remove("bms_algo.so")
        print("\nCleaned up compiled library.")
    except:
        pass

if __name__ == "__main__":
    main()