/**
 * @file servo_control.h
 * @brief Servo motor control for hand prosthesis gestures
 */

#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"

/* Exported types ------------------------------------------------------------*/
// Servo channel configuration
typedef struct {
    uint32_t timer_channel;   // TIM_CHANNEL_x
    uint16_t min_pulse_us;    // Minimum pulse width in microseconds
    uint16_t max_pulse_us;    // Maximum pulse width in microseconds
    uint16_t center_pulse_us; // Center/neutral pulse width
    uint8_t min_angle;        // Minimum angle (typically 0)
    uint8_t max_angle;        // Maximum angle (typically 180)
    uint8_t current_angle;    // Current position
    uint8_t target_angle;     // Target position
    bool inverted;            // Reverse direction if true
} Servo_Channel_t;

// Servo controller state
typedef struct {
    TIM_HandleTypeDef *htim;  // Timer handle for PWM generation
    Servo_Channel_t channels[6]; // 6 servo channels
    bool initialized;         // Initialization status
    bool emergency_stop;      // Emergency stop flag
} Servo_Controller_t;

// Gesture definition
typedef struct {
    char name[32];            // Gesture name (e.g., "A", "B", "C")
    uint8_t servo_angles[6];  // Target angles for each servo
    uint16_t transition_time; // Time to complete gesture (ms)
} Gesture_Definition_t;

/* Exported constants --------------------------------------------------------*/
// Servo indices
#define SERVO_THUMB     0
#define SERVO_INDEX     1
#define SERVO_MIDDLE    2
#define SERVO_RING      3
#define SERVO_PINKY     4
#define SERVO_WRIST     5

// PWM parameters
#define SERVO_PWM_FREQUENCY    50     // 50 Hz (20ms period)
#define SERVO_MIN_PULSE        1000   // 1.0 ms
#define SERVO_CENTER_PULSE     1500   // 1.5 ms
#define SERVO_MAX_PULSE        2000   // 2.0 ms

// Angle limits
#define SERVO_MIN_ANGLE        0
#define SERVO_MAX_ANGLE        180
#define SERVO_CENTER_ANGLE     90

// Gesture IDs for initial 3 classes
#define GESTURE_OPEN_HAND      0
#define GESTURE_CLOSED_FIST    1
#define GESTURE_PEACE_SIGN     2

/* Exported functions prototypes ---------------------------------------------*/
// Initialization
HAL_StatusTypeDef Servo_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef Servo_ConfigureChannel(uint8_t channel, const Servo_Channel_t *config);
HAL_StatusTypeDef Servo_Start(void);
HAL_StatusTypeDef Servo_Stop(void);

// Basic control
HAL_StatusTypeDef Servo_SetAngle(uint8_t channel, uint8_t angle);
HAL_StatusTypeDef Servo_SetMultipleAngles(const uint8_t angles[6]);
uint8_t Servo_GetAngle(uint8_t channel);
HAL_StatusTypeDef Servo_SetPulseWidth(uint8_t channel, uint16_t pulse_us);

// Gesture control
HAL_StatusTypeDef Servo_ExecuteGesture(uint8_t gesture_id);
HAL_StatusTypeDef Servo_GetGesturePositions(uint8_t gesture_id, uint8_t positions[6]);
HAL_StatusTypeDef Servo_DefineGesture(uint8_t gesture_id, const Gesture_Definition_t *gesture);
const char* Servo_GetGestureName(uint8_t gesture_id);

// Smooth movement
HAL_StatusTypeDef Servo_MoveToAngle(uint8_t channel, uint8_t target_angle, uint16_t duration_ms);
HAL_StatusTypeDef Servo_MoveToGesture(uint8_t gesture_id, uint16_t duration_ms);
bool Servo_IsMoving(uint8_t channel);
void Servo_UpdateMovement(void);  // Call periodically for smooth movement

// Safety functions
void Servo_EmergencyStop(void);
void Servo_ReleaseEmergencyStop(void);
bool Servo_IsEmergencyStopped(void);
HAL_StatusTypeDef Servo_SetSafePosition(void);

// Calibration
HAL_StatusTypeDef Servo_CalibrateChannel(uint8_t channel, uint16_t min_pulse, uint16_t max_pulse);
HAL_StatusTypeDef Servo_SaveCalibration(void);
HAL_StatusTypeDef Servo_LoadCalibration(void);

// Power management
HAL_StatusTypeDef Servo_EnableChannel(uint8_t channel, bool enable);
HAL_StatusTypeDef Servo_SetLowPowerMode(bool enable);
float Servo_GetEstimatedCurrent(void);

// Diagnostics
HAL_StatusTypeDef Servo_TestChannel(uint8_t channel);
HAL_StatusTypeDef Servo_TestAllChannels(void);
void Servo_GetStatus(uint8_t channel, bool *enabled, uint8_t *angle, bool *moving);

#ifdef __cplusplus
}
#endif

#endif /* SERVO_CONTROL_H */