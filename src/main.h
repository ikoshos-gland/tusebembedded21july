/**
 * @file main.h
 * @brief Main application header file
 */

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* Exported types ------------------------------------------------------------*/
typedef enum {
    MODE_IDLE,
    MODE_ACTIVE,
    MODE_CALIBRATION,
    MODE_DEBUG
} System_Mode_t;

typedef enum {
    ERROR_NONE = 0,
    ERROR_EMG_TIMEOUT,
    ERROR_EMG_OVERFLOW,
    ERROR_ACC_COMM,
    ERROR_SERVO_FAULT,
    ERROR_ML_MEMORY,
    ERROR_ML_INFERENCE,
    ERROR_BATTERY_LOW
} System_Error_t;

typedef struct {
    uint32_t emg_sample_rate;
    uint32_t dsp_processing_time;
    uint32_t ml_inference_time;
    uint32_t total_predictions;
    uint32_t dropped_samples;
} System_Stats_t;

typedef struct {
    System_Mode_t mode;
    System_Error_t error_code;
    uint8_t current_gesture;
    uint8_t gesture_confidence;
    float battery_voltage;
    float temperature;
    bool debug_enabled;
    System_Stats_t stats;
} System_State_t;

/* Exported constants --------------------------------------------------------*/
#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  0
#define FIRMWARE_VERSION_PATCH  0

/* Exported macro ------------------------------------------------------------*/
#define UNUSED(x) ((void)(x))

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */