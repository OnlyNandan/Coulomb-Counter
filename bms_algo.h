#ifndef BMS_ALGO_H
#define BMS_ALGO_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float soc_percent;              // State of Charge (0-100%)
    int64_t coulomb_count_uAs;     // Fixed-point charge tracking (microampere-seconds)
    float current_capacity_ah;      // Current battery capacity (Ah)
    float nominal_capacity_ah;      // Nominal battery capacity (Ah)
    
    float kalman_gain;              // Kalman gain
    float process_noise;            // Process noise variance
    float measurement_noise;         // Measurement noise variance
    float error_covariance;          // Error covariance
    
    float soh_percent;              // State of Health (0-100%)
    float capacity_adaptation_rate;  // Rate of capacity adaptation
    float soc_error_accumulator;     // Accumulated SOC error for SOH
    uint32_t soh_update_count;      // Counter for SOH adaptation triggers
    
    uint32_t update_count;          // Number of updates performed
    bool rest_period_active;        // Flag for rest period detection
    float rest_period_timer;        // Timer for rest period detection
    bool correction_has_been_applied; // Flag to track if correction applied during current rest period
    
    float last_update_time_us;      // Last update execution time (microseconds)
} BMS_State;

void BMS_Init(BMS_State* state, float initial_soc_percent, float nominal_capacity_ah);
void BMS_Update(BMS_State* state, float voltage, float current, float temperature, float dt_seconds);

float BMS_GetCurrent(float adc_ch1_volts, float adc_ch2_volts);
float BMS_GetOCVSOC(float voltage, float temperature);
float BMS_GetInternalResistance(float soc, float temp);
float BMS_BilinearInterpolate(float x, float y, const float* x_axis, const float* y_axis, 
                              const float* table, int x_size, int y_size);

#define MAX_CURRENT_CH1_A 70.0f
#define COULOMB_SCALE_FACTOR 1e6f
#define REST_PERIOD_THRESHOLD 0.1f
#define REST_PERIOD_TIME 5.0f
#define CAPACITY_ADAPTATION_RATE 0.00005f

#endif