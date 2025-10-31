# Battery Management System (BMS) Algorithm Simulator

A complete simulation environment for a sophisticated Battery Management System algorithm designed for EV racing teams. This project consists of an optimized C library with core BMS logic and a Python test harness for simulation, testing, and visualization.

## Project Overview

The BMS algorithm implements:
- **SOC Estimation**: Core Coulomb Counter corrected by a 1D Kalman Filter
- **Temperature Compensation**: Kalman filter uses a 2D lookup table for Open Circuit Voltage (OCV) from SOC and Temperature
- **SOH Estimation**: Capacity adaptation algorithm using error between Coulomb count SOC and OCV-based SOC during rest periods

## Files Structure

```
Coulomb Counter/
├── bms_algo.h          # Header file with BMS_State struct and function prototypes
├── bms_algo.c          # Core BMS implementation with optimized algorithms
├── simulator.py        # Python test harness with compilation and visualization
└── README.md           # This documentation file
```

## Features

### C Library (`bms_algo.h` and `bms_algo.c`)

**Memory & Optimization Constraints:**
- No dynamic memory allocation (malloc/free)
- Fixed-point arithmetic for Coulomb counter using int64_t (microampere-seconds)
- All state variables contained in single BMS_State struct
- Static memory allocation only

**Core Functions:**
- `BMS_Init()`: Initialize BMS state with initial SOC and nominal capacity
- `BMS_Update()`: Main update function for periodic calls
- `BMS_GetCurrent()`: LEM DHAB S/124 sensor driver with dual-channel processing
- `BMS_GetOCVSOC()`: 2D lookup table for SOC estimation from voltage and temperature

**Sensor Driver:**
- Channel 1: High Sensitivity (±75A, 26.7 mV/A, 2.5V offset)
- Channel 2: High Range (±500A, 4 mV/A, 2.5V offset)
- Automatic switching based on current threshold (70A)

**2D Lookup Table:**
- 100 voltage points (12.058V to 13.418V)
- 7 temperature points (263K to 313K)
- Bilinear interpolation for accurate SOC estimation

### Python Simulator (`simulator.py`)

**Compilation & Interface:**
- Automatic C library compilation using subprocess
- ctypes integration for calling C functions
- Cross-platform support (Windows/Linux/macOS)

**Simulation Features:**
- Realistic drive cycle generation (charge, discharge, rest periods)
- Performance timing analysis
- Memory usage calculation
- Lifecycle simulation for SOH degradation tracking

**Visualization:**
- Multi-panel main analysis plot (SOC, SOH, Current, Voltage)
- Performance analysis (execution time histogram and trends)
- Lifecycle analysis (SOH vs Cycle Number)

## Usage

### Prerequisites

- Python 3.7+
- GCC compiler
- Required Python packages:
  ```bash
  pip install numpy matplotlib
  ```

### Running the Simulator

1. **Clone or download the project files**
2. **Run the simulator:**
   ```bash
   python simulator.py
   ```

The simulator will:
1. Compile the C library automatically
2. Run a 300-second simulation with realistic drive cycles
3. Generate performance analysis plots
4. Run a 100-cycle lifecycle simulation
5. Create visualization plots and save them as PNG files

### Generated Outputs

The simulator generates three main visualization files:
- `bms_main_analysis.png`: Main multi-panel analysis
- `bms_performance_analysis.png`: Performance timing analysis
- `bms_lifecycle_analysis.png`: SOH degradation over lifecycle

## Algorithm Details

### SOC Estimation
1. **Coulomb Counter**: Fixed-point arithmetic tracks charge in microampere-seconds
2. **Kalman Filter**: Corrects Coulomb counter using OCV-based SOC from lookup table
3. **Temperature Compensation**: Lookup table accounts for temperature effects on OCV

### SOH Estimation
1. **Rest Period Detection**: Monitors current below threshold (0.1A) for 5 seconds
2. **Error Accumulation**: Tracks difference between Coulomb SOC and OCV SOC
3. **Capacity Adaptation**: Gradually adjusts total capacity based on accumulated error
4. **SOH Calculation**: SOH = (Current Capacity / Nominal Capacity) × 100%

### Performance Characteristics
- **Memory Usage**: ~3.2KB total static memory
- **Execution Time**: Typically <10μs per update
- **Update Rate**: Designed for 10Hz operation (100ms intervals)

## Technical Specifications

### BMS_State Structure
```c
typedef struct {
    float soc_percent;              // State of Charge (0-100%)
    int64_t coulomb_count_uAs;     // Fixed-point charge tracking
    float current_capacity_ah;      // Current battery capacity
    float nominal_capacity_ah;      // Nominal battery capacity
    float kalman_gain;              // Kalman filter gain
    float soh_percent;              // State of Health (0-100%)
    // ... additional parameters
} BMS_State;
```

### Key Constants
- `MAX_CURRENT_CH1_A`: 70.0A (Channel 1 saturation threshold)
- `COULOMB_SCALE_FACTOR`: 1e6 (Microampere-seconds scaling)
- `REST_PERIOD_THRESHOLD`: 0.1A (Rest period detection)
- `REST_PERIOD_TIME`: 5.0s (Required rest duration)
- `CAPACITY_ADAPTATION_RATE`: 0.001 (Adaptation rate per update)

## Customization

### Modifying Drive Cycles
Edit the `generate_drive_cycle()` function in `simulator.py` to create custom current/voltage profiles.

### Adjusting Algorithm Parameters
Modify constants in `bms_algo.h` to tune algorithm behavior:
- Kalman filter noise parameters
- Rest period detection thresholds
- Capacity adaptation rates

### Adding New Features
The modular design allows easy extension:
- Additional sensor inputs
- Enhanced SOH algorithms
- Custom visualization plots

## Performance Analysis

The simulator provides comprehensive performance analysis:
- **Execution Time**: Histogram and time-series plots
- **Memory Usage**: Detailed breakdown of static memory allocation
- **Algorithm Accuracy**: SOC/SOH estimation performance over time
- **Lifecycle Behavior**: Long-term degradation modeling

## License

This project is provided as-is for educational and research purposes. Please ensure compliance with any applicable licenses for embedded systems use.

## Contributing

Contributions are welcome! Areas for improvement:
- Enhanced lookup table accuracy
- Additional sensor support
- Advanced SOH algorithms
- Real-time visualization capabilities
