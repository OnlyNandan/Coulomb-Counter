#include "bms_algo.h"
#include <math.h>
#include <string.h>

const float V_LOOKUP_AXIS[100] = {
    12.0584f, 12.0721f, 12.0858f, 12.0995f, 12.1132f, 12.1269f, 12.1406f, 12.1543f, 12.1680f, 12.1817f,
    12.1954f, 12.2091f, 12.2228f, 12.2365f, 12.2502f, 12.2639f, 12.2776f, 12.2913f, 12.3050f, 12.3187f,
    12.3324f, 12.3461f, 12.3598f, 12.3735f, 12.3872f, 12.4009f, 12.4146f, 12.4283f, 12.4420f, 12.4557f,
    12.4694f, 12.4831f, 12.4968f, 12.5105f, 12.5242f, 12.5379f, 12.5516f, 12.5653f, 12.5790f, 12.5927f,
    12.6064f, 12.6201f, 12.6338f, 12.6475f, 12.6612f, 12.6749f, 12.6886f, 12.7023f, 12.7160f, 12.7297f,
    12.7434f, 12.7571f, 12.7708f, 12.7845f, 12.7982f, 12.8119f, 12.8256f, 12.8393f, 12.8530f, 12.8667f,
    12.8804f, 12.8941f, 12.9078f, 12.9215f, 12.9352f, 12.9489f, 12.9626f, 12.9763f, 12.9900f, 13.0037f,
    13.0174f, 13.0311f, 13.0448f, 13.0585f, 13.0722f, 13.0859f, 13.0996f, 13.1133f, 13.1270f, 13.1407f,
    13.1544f, 13.1681f, 13.1818f, 13.1955f, 13.2092f, 13.2229f, 13.2366f, 13.2503f, 13.2640f, 13.2777f,
    13.2914f, 13.3051f, 13.3188f, 13.3325f, 13.3462f, 13.3599f, 13.3736f, 13.3873f, 13.4010f, 13.4147f
};

const float T_LOOKUP_AXIS[7] = { 263.0f, 273.0f, 283.0f, 293.0f, 296.0f, 303.0f, 313.0f };

const float SOC_LOOKUP_AXIS[10] = { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f };

const float SOC_LOOKUP_TABLE[100][7] = {
    {0.00f, 0.20f, 0.40f, 0.50f, 0.40f, 0.20f, 0.00f},
    {1.01f, 1.21f, 1.41f, 1.51f, 1.41f, 1.21f, 1.01f},
    {2.02f, 2.22f, 2.42f, 2.52f, 2.42f, 2.22f, 2.02f},
    {3.03f, 3.23f, 3.43f, 3.53f, 3.43f, 3.23f, 3.03f},
    {4.04f, 4.24f, 4.44f, 4.54f, 4.44f, 4.24f, 4.04f},
    {5.05f, 5.25f, 5.45f, 5.55f, 5.45f, 5.25f, 5.05f},
    {6.06f, 6.26f, 6.46f, 6.56f, 6.46f, 6.26f, 6.06f},
    {7.07f, 7.27f, 7.47f, 7.57f, 7.47f, 7.27f, 7.07f},
    {8.08f, 8.28f, 8.48f, 8.58f, 8.48f, 8.28f, 8.08f},
    {9.09f, 9.29f, 9.49f, 9.59f, 9.49f, 9.29f, 9.09f},
    {10.10f, 10.30f, 10.50f, 10.60f, 10.50f, 10.30f, 10.10f},
    {11.11f, 11.31f, 11.51f, 11.61f, 11.51f, 11.31f, 11.11f},
    {12.12f, 12.32f, 12.52f, 12.62f, 12.52f, 12.32f, 12.12f},
    {13.13f, 13.33f, 13.53f, 13.63f, 13.53f, 13.33f, 13.13f},
    {14.14f, 14.34f, 14.54f, 14.64f, 14.54f, 14.34f, 14.14f},
    {15.15f, 15.35f, 15.55f, 15.65f, 15.55f, 15.35f, 15.15f},
    {16.16f, 16.36f, 16.56f, 16.66f, 16.56f, 16.36f, 16.16f},
    {17.17f, 17.37f, 17.57f, 17.67f, 17.57f, 17.37f, 17.17f},
    {18.18f, 18.38f, 18.58f, 18.68f, 18.58f, 18.38f, 18.18f},
    {19.19f, 19.39f, 19.59f, 19.69f, 19.59f, 19.39f, 19.19f},
    {20.20f, 20.40f, 20.60f, 20.70f, 20.60f, 20.40f, 20.20f},
    {21.21f, 21.41f, 21.61f, 21.71f, 21.61f, 21.41f, 21.21f},
    {22.22f, 22.42f, 22.62f, 22.72f, 22.62f, 22.42f, 22.22f},
    {23.23f, 23.43f, 23.63f, 23.73f, 23.63f, 23.43f, 23.23f},
    {24.24f, 24.44f, 24.64f, 24.74f, 24.64f, 24.44f, 24.24f},
    {25.25f, 25.45f, 25.65f, 25.75f, 25.65f, 25.45f, 25.25f},
    {26.26f, 26.46f, 26.66f, 26.76f, 26.66f, 26.46f, 26.26f},
    {27.27f, 27.47f, 27.67f, 27.77f, 27.67f, 27.47f, 27.27f},
    {28.28f, 28.48f, 28.68f, 28.78f, 28.68f, 28.48f, 28.28f},
    {29.29f, 29.49f, 29.69f, 29.79f, 29.69f, 29.49f, 29.29f},
    {30.30f, 30.50f, 30.70f, 30.80f, 30.70f, 30.50f, 30.30f},
    {31.31f, 31.51f, 31.71f, 31.81f, 31.71f, 31.51f, 31.31f},
    {32.32f, 32.52f, 32.72f, 32.82f, 32.72f, 32.52f, 32.32f},
    {33.33f, 33.53f, 33.73f, 33.83f, 33.73f, 33.53f, 33.33f},
    {34.34f, 34.54f, 34.74f, 34.84f, 34.74f, 34.54f, 34.34f},
    {35.35f, 35.55f, 35.75f, 35.85f, 35.75f, 35.55f, 35.35f},
    {36.36f, 36.56f, 36.76f, 36.86f, 36.76f, 36.56f, 36.36f},
    {37.37f, 37.57f, 37.77f, 37.87f, 37.77f, 37.57f, 37.37f},
    {38.38f, 38.58f, 38.78f, 38.88f, 38.78f, 38.58f, 38.38f},
    {39.39f, 39.59f, 39.79f, 39.89f, 39.79f, 39.59f, 39.39f},
    {40.40f, 40.60f, 40.80f, 40.90f, 40.80f, 40.60f, 40.40f},
    {41.41f, 41.61f, 41.81f, 41.91f, 41.81f, 41.61f, 41.41f},
    {42.42f, 42.62f, 42.82f, 42.92f, 42.82f, 42.62f, 42.42f},
    {43.43f, 43.63f, 43.83f, 43.93f, 43.83f, 43.63f, 43.43f},
    {44.44f, 44.64f, 44.84f, 44.94f, 44.84f, 44.64f, 44.44f},
    {45.45f, 45.65f, 45.85f, 45.95f, 45.85f, 45.65f, 45.45f},
    {46.46f, 46.66f, 46.86f, 46.96f, 46.86f, 46.66f, 46.46f},
    {47.47f, 47.67f, 47.87f, 47.97f, 47.87f, 47.67f, 47.47f},
    {48.48f, 48.68f, 48.88f, 48.98f, 48.88f, 48.68f, 48.48f},
    {49.49f, 49.69f, 49.89f, 49.99f, 49.89f, 49.69f, 49.49f},
    {50.51f, 50.71f, 50.91f, 51.01f, 50.91f, 50.71f, 50.51f},
    {51.52f, 51.72f, 51.92f, 52.02f, 51.92f, 51.72f, 51.52f},
    {52.53f, 52.73f, 52.93f, 53.03f, 52.93f, 52.73f, 52.53f},
    {53.54f, 53.74f, 53.94f, 54.04f, 53.94f, 53.74f, 53.54f},
    {54.55f, 54.75f, 54.95f, 55.05f, 54.95f, 54.75f, 54.55f},
    {55.56f, 55.76f, 55.96f, 56.06f, 55.96f, 55.76f, 55.56f},
    {56.57f, 56.77f, 56.97f, 57.07f, 56.97f, 56.77f, 56.57f},
    {57.58f, 57.78f, 57.98f, 58.08f, 57.98f, 57.78f, 57.58f},
    {58.59f, 58.79f, 58.99f, 59.09f, 58.99f, 58.79f, 58.59f},
    {59.60f, 59.80f, 60.00f, 60.10f, 60.00f, 59.80f, 59.60f},
    {60.61f, 60.81f, 61.01f, 61.11f, 61.01f, 60.81f, 60.61f},
    {61.62f, 61.82f, 62.02f, 62.12f, 62.02f, 61.82f, 61.62f},
    {62.63f, 62.83f, 63.03f, 63.13f, 63.03f, 62.83f, 62.63f},
    {63.64f, 63.84f, 64.04f, 64.14f, 64.04f, 63.84f, 63.64f},
    {64.65f, 64.85f, 65.05f, 65.15f, 65.05f, 64.85f, 64.65f},
    {65.66f, 65.86f, 66.06f, 66.16f, 66.06f, 65.86f, 65.66f},
    {66.67f, 66.87f, 67.07f, 67.17f, 67.07f, 66.87f, 66.67f},
    {67.68f, 67.88f, 68.08f, 68.18f, 68.08f, 67.88f, 67.68f},
    {68.69f, 68.89f, 69.09f, 69.19f, 69.09f, 68.89f, 68.69f},
    {69.70f, 69.90f, 70.10f, 70.20f, 70.10f, 69.90f, 69.70f},
    {70.71f, 70.91f, 71.11f, 71.21f, 71.11f, 70.91f, 70.71f},
    {71.72f, 71.92f, 72.12f, 72.22f, 72.12f, 71.92f, 71.72f},
    {72.73f, 72.93f, 73.13f, 73.23f, 73.13f, 72.93f, 72.73f},
    {73.74f, 73.94f, 74.14f, 74.24f, 74.14f, 73.94f, 73.74f},
    {74.75f, 74.95f, 75.15f, 75.25f, 75.15f, 74.95f, 74.75f},
    {75.76f, 75.96f, 76.16f, 76.26f, 76.16f, 75.96f, 75.76f},
    {76.77f, 76.97f, 77.17f, 77.27f, 77.17f, 76.97f, 76.77f},
    {77.78f, 77.98f, 78.18f, 78.28f, 78.18f, 77.98f, 77.78f},
    {78.79f, 78.99f, 79.19f, 79.29f, 79.19f, 78.99f, 78.79f},
    {79.80f, 80.00f, 80.20f, 80.30f, 80.20f, 80.00f, 79.80f},
    {80.81f, 81.01f, 81.21f, 81.31f, 81.21f, 81.01f, 80.81f},
    {81.82f, 82.02f, 82.22f, 82.32f, 82.22f, 82.02f, 81.82f},
    {82.83f, 83.03f, 83.23f, 83.33f, 83.23f, 83.03f, 82.83f},
    {83.84f, 84.04f, 84.24f, 84.34f, 84.24f, 84.04f, 83.84f},
    {84.85f, 85.05f, 85.25f, 85.35f, 85.25f, 85.05f, 84.85f},
    {85.86f, 86.06f, 86.26f, 86.36f, 86.26f, 86.06f, 85.86f},
    {86.87f, 87.07f, 87.27f, 87.37f, 87.27f, 87.07f, 86.87f},
    {87.88f, 88.08f, 88.28f, 88.38f, 88.28f, 88.08f, 87.88f},
    {88.89f, 89.09f, 89.29f, 89.39f, 89.29f, 89.09f, 88.89f},
    {89.90f, 90.10f, 90.30f, 90.40f, 90.30f, 90.10f, 89.90f},
    {90.91f, 91.11f, 91.31f, 91.41f, 91.31f, 91.11f, 90.91f},
    {91.92f, 92.12f, 92.32f, 92.42f, 92.32f, 92.12f, 91.92f},
    {92.93f, 93.13f, 93.33f, 93.43f, 93.33f, 93.13f, 92.93f},
    {93.94f, 94.14f, 94.34f, 94.44f, 94.34f, 94.14f, 93.94f},
    {94.95f, 95.15f, 95.35f, 95.45f, 95.35f, 95.15f, 94.95f},
    {95.96f, 96.16f, 96.36f, 96.46f, 96.36f, 96.16f, 95.96f},
    {96.97f, 97.17f, 97.37f, 97.47f, 97.37f, 97.17f, 96.97f},
    {97.98f, 98.18f, 98.38f, 98.48f, 98.38f, 98.18f, 97.98f},
    {98.99f, 99.19f, 99.39f, 99.49f, 99.39f, 99.19f, 98.99f},
    {100.00f, 100.00f, 100.00f, 100.00f, 100.00f, 100.00f, 100.00f},
};



const float R_INTERNAL_LOOKUP_TABLE[10][7] = {
    {0.050f, 0.045f, 0.040f, 0.035f, 0.033f, 0.030f, 0.025f},
    {0.045f, 0.040f, 0.035f, 0.030f, 0.028f, 0.025f, 0.020f},
    {0.040f, 0.035f, 0.030f, 0.025f, 0.023f, 0.020f, 0.015f},
    {0.035f, 0.030f, 0.025f, 0.020f, 0.018f, 0.015f, 0.012f},
    {0.030f, 0.025f, 0.020f, 0.015f, 0.013f, 0.010f, 0.008f},
    {0.025f, 0.020f, 0.015f, 0.010f, 0.008f, 0.005f, 0.003f},
    {0.020f, 0.015f, 0.010f, 0.005f, 0.003f, 0.002f, 0.001f},
    {0.018f, 0.013f, 0.008f, 0.003f, 0.001f, 0.000f, 0.000f},
    {0.015f, 0.010f, 0.005f, 0.000f, 0.000f, 0.000f, 0.000f},
    {0.012f, 0.007f, 0.002f, 0.000f, 0.000f, 0.000f, 0.000f}
};

float BMS_GetCurrent(float adc_ch1_volts, float adc_ch2_volts) {
    float current_ch1 = (adc_ch1_volts - 2.5f) / 0.0267f;
    
    float current_ch2 = (adc_ch2_volts - 2.5f) / 0.004f;
    
    if (fabsf(current_ch1) <= MAX_CURRENT_CH1_A) {
        return current_ch1;
    } else {
        return current_ch2;
    }
}

float BMS_BilinearInterpolate(float x, float y, const float* x_axis, const float* y_axis, 
                              const float* table, int x_size, int y_size) {
    int x_idx = 0;
    while (x_idx < x_size - 2 && x > x_axis[x_idx + 1]) {
        x_idx++;
    }
    
    int y_idx = 0;
    while (y_idx < y_size - 2 && y > y_axis[y_idx + 1]) {
        y_idx++;
    }
    
    float x1 = x_axis[x_idx];
    float x2 = x_axis[x_idx + 1];
    float y1 = y_axis[y_idx];
    float y2 = y_axis[y_idx + 1];
    
    float q11 = table[x_idx * y_size + y_idx];
    float q12 = table[x_idx * y_size + (y_idx + 1)];
    float q21 = table[(x_idx + 1) * y_size + y_idx];
    float q22 = table[(x_idx + 1) * y_size + (y_idx + 1)];
    
    if ((x2 - x1) == 0.0f || (y2 - y1) == 0.0f) {
        return q11;
    }

    float f_x1_y = ((y2 - y) / (y2 - y1)) * q11 + ((y - y1) / (y2 - y1)) * q12;
    float f_x2_y = ((y2 - y) / (y2 - y1)) * q21 + ((y - y1) / (y2 - y1)) * q22;

    return ((x2 - x) / (x2 - x1)) * f_x1_y + ((x - x1) / (x2 - x1)) * f_x2_y;
}

float BMS_GetOCVSOC(float voltage, float temperature) {
    return BMS_BilinearInterpolate(voltage, temperature, V_LOOKUP_AXIS, T_LOOKUP_AXIS, 
                                   (const float*)SOC_LOOKUP_TABLE, 100, 7);
}

float BMS_GetInternalResistance(float soc, float temp) {
    return BMS_BilinearInterpolate(soc, temp, SOC_LOOKUP_AXIS, T_LOOKUP_AXIS, 
                                   (const float*)R_INTERNAL_LOOKUP_TABLE, 10, 7);
}

void BMS_Init(BMS_State* state, float initial_soc_percent, float nominal_capacity_ah) {
    if (state == NULL) return;
    
    state->soc_percent = initial_soc_percent;
    state->coulomb_count_uAs = (int64_t)(initial_soc_percent * nominal_capacity_ah * 3600.0f * COULOMB_SCALE_FACTOR / 100.0f);
    state->current_capacity_ah = nominal_capacity_ah;
    state->nominal_capacity_ah = nominal_capacity_ah;
    
    state->kalman_gain = 0.1f;
    state->process_noise = 0.00001f;
    state->measurement_noise = 15.0f;
    state->error_covariance = 0.1f;
    
    state->soh_percent = 100.0f;
    state->capacity_adaptation_rate = CAPACITY_ADAPTATION_RATE;
    state->soc_error_accumulator = 0.0f;
    state->soh_update_count = 0;
    
    state->update_count = 0;
    state->rest_period_active = false;
    state->rest_period_timer = 0.0f;
    state->correction_has_been_applied = false;
    
    state->last_update_time_us = 0.0f;
}

void BMS_Update(BMS_State* state, float voltage, float current, float temperature, float dt_seconds) {
    if (state == NULL || dt_seconds <= 0.0f) return;
    
    int64_t current_uAs = (int64_t)(current * dt_seconds * COULOMB_SCALE_FACTOR);
    state->coulomb_count_uAs += current_uAs;
    
    float coulomb_soc = (float)(state->coulomb_count_uAs) / (state->current_capacity_ah * 3600.0f * COULOMB_SCALE_FACTOR) * 100.0f;
    
    if (coulomb_soc < 0.0f) coulomb_soc = 0.0f;
    if (coulomb_soc > 100.0f) coulomb_soc = 100.0f;
    
    float ocv_soc = BMS_GetOCVSOC(voltage, temperature);
    
    float predicted_soc = coulomb_soc;
    
    float predicted_p = state->error_covariance + state->process_noise;
    
    state->kalman_gain = predicted_p / (predicted_p + state->measurement_noise);
    
    state->soc_percent = predicted_soc + state->kalman_gain * (ocv_soc - predicted_soc);
    
    state->error_covariance = (1.0f - state->kalman_gain) * predicted_p;
    
    if (state->soc_percent < 0.0f) state->soc_percent = 0.0f;
    if (state->soc_percent > 100.0f) state->soc_percent = 100.0f;
    
    if (fabsf(current) < REST_PERIOD_THRESHOLD) {
        if (!state->rest_period_active) {
            state->rest_period_active = true;
            state->rest_period_timer = 0.0f;
        }
        state->rest_period_timer += dt_seconds;
        
        if (state->rest_period_timer >= REST_PERIOD_TIME && !state->correction_has_been_applied) {
            state->soh_update_count++;
            
            float soc_error = ocv_soc - coulomb_soc;
            state->current_capacity_ah += soc_error * state->nominal_capacity_ah / 100.0f * (state->capacity_adaptation_rate * 10.0f);
            
            if (state->current_capacity_ah < 0.5f * state->nominal_capacity_ah) {
                state->current_capacity_ah = 0.5f * state->nominal_capacity_ah;
            }
            if (state->current_capacity_ah > 1.2f * state->nominal_capacity_ah) {
                state->current_capacity_ah = 1.2f * state->nominal_capacity_ah;
            }
            
            state->soh_percent = (state->current_capacity_ah / state->nominal_capacity_ah) * 100.0f;
            
            
            
            state->correction_has_been_applied = true;
        }
    } else {
        state->rest_period_active = false;
        state->rest_period_timer = 0.0f;
        state->correction_has_been_applied = false;
    }
    
    state->update_count++;
}