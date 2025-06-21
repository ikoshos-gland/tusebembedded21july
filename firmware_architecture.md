# sEMG Hand Prosthesis - Firmware Architecture

## System Overview

The firmware implements a real-time signal processing pipeline for sEMG-based gesture recognition using the STM32H7S3L8 microcontroller.

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│  ADS1299    │────▶│   STM32H7    │────▶│  6 Servos   │
│  (4ch EMG)  │ SPI │              │ PWM │             │
└─────────────┘     │              │     └─────────────┘
                    │              │
┌─────────────┐     │              │
│   LIS3DH    │────▶│              │
│ (1-axis acc)│ I2C │              │
└─────────────┘     └──────────────┘
```

## Memory Map

### RAM Allocation (Target: <12KB for ML)
```
┌─────────────────────────┐ 0x2000_0000
│   Stack (4KB)           │
├─────────────────────────┤ 0x2000_1000
│   EMG Buffers (8KB)     │ Double buffer: 2×256×4×4
├─────────────────────────┤ 0x2000_3000
│   FFT Working (2KB)     │ 64-point complex
├─────────────────────────┤ 0x2000_3800
│   Features (1KB)        │ ~30 features × 4 bytes
├─────────────────────────┤ 0x2000_3C00
│   RF Working (4KB)      │ Tree traversal
├─────────────────────────┤ 0x2000_4C00
│   Voting Buffer (256B)  │ 3 predictions
├─────────────────────────┤ 0x2000_4D00
│   System/Heap           │
└─────────────────────────┘
```

### Flash Allocation (Target: <32KB for ML)
```
┌─────────────────────────┐ 0x0800_0000
│   Bootloader (16KB)     │
├─────────────────────────┤ 0x0800_4000
│   Application Code      │
├─────────────────────────┤ 0x0802_0000
│   RF Model (32KB)       │ XIP-optimized
├─────────────────────────┤ 0x0802_8000
│   Calibration Data      │
└─────────────────────────┘
```

## Core Modules

### 1. Data Acquisition Module

```c
// emg_acquisition.h
typedef struct {
    uint32_t sample_rate;      // 1000 Hz default
    uint8_t  channels;         // 4
    uint8_t  resolution;       // 24-bit
    uint32_t buffer_size;      // 256 samples
} EMG_Config_t;

typedef struct {
    int32_t data[4];          // 4 channels
    uint32_t timestamp;       // System tick
} EMG_Sample_t;

// Circular buffer with DMA
typedef struct {
    EMG_Sample_t buffer[512]; // Double buffer
    uint16_t write_idx;
    uint16_t read_idx;
    uint8_t half_complete;
    uint8_t full_complete;
} EMG_Buffer_t;
```

### 2. Signal Processing Module

```c
// dsp_pipeline.h
typedef struct {
    float32_t window[64];     // Hamming coefficients
    float32_t fft_in[128];    // Complex input
    float32_t fft_out[128];   // Complex output
    float32_t magnitude[64];  // Magnitude spectrum
} STFT_Context_t;

typedef struct {
    float32_t rms;
    float32_t mav;
    float32_t var;
    float32_t zc;
    float32_t ssc;
    float32_t wl;
    float32_t mpf;
    float32_t mdf;
    float32_t power_bands[4]; // 0-50, 50-150, 150-250, 250-500 Hz
} Feature_Vector_t;
```

### 3. Random Forest Classifier

```c
// random_forest.h
typedef struct {
    uint8_t feature_idx;
    int16_t threshold;        // Q8.8 fixed-point
    uint8_t left_child;
    uint8_t right_child;
    uint8_t class_label;      // For leaf nodes
} RF_Node_t;

typedef struct {
    RF_Node_t nodes[64];      // Max 64 nodes per tree
    uint8_t n_nodes;
    uint8_t root_idx;
} RF_Tree_t;

typedef struct {
    RF_Tree_t trees[20];      // 20 trees max
    uint8_t n_trees;
    uint8_t n_features;
    uint8_t n_classes;
    float32_t feature_scale[30];
    float32_t feature_offset[30];
} RF_Model_t;
```

### 4. Servo Control Module

```c
// servo_control.h
typedef struct {
    TIM_HandleTypeDef* timer;
    uint32_t channel;
    uint16_t min_pulse;       // μs
    uint16_t max_pulse;       // μs
    uint8_t current_angle;
    uint8_t target_angle;
    uint8_t speed;            // degrees/sec
} Servo_Channel_t;

typedef struct {
    Servo_Channel_t servos[6];
    uint8_t gesture_map[29][6]; // Gesture to servo angles
} Servo_Controller_t;
```

## Real-Time Task Schedule

### Task Priorities (FreeRTOS)
```
┌─────────────────────────────┬──────────┬────────┐
│ Task                        │ Priority │ Period │
├─────────────────────────────┼──────────┼────────┤
│ EMG_AcquisitionTask         │ 5 (High) │ 1ms    │
│ DSP_ProcessingTask          │ 4        │ 128ms  │
│ ML_InferenceTask            │ 3        │ 128ms  │
│ Servo_ControlTask           │ 2        │ 20ms   │
│ System_MonitorTask          │ 1 (Low)  │ 1000ms │
└─────────────────────────────┴──────────┴────────┘
```

### Timing Diagram
```
Time (ms): 0    128   256   384   512
           │     │     │     │     │
EMG:       ████████████████████████  (continuous)
           │     │     │     │
Window:    |--W1-|--W2-|--W3-|       (50% overlap)
           │     │     │     │
STFT:      └─F1──┴─F2──┴─F3──┘       (3 windows)
                 │     │     │
Inference:       └─RF──┴─RF──┘       (on each window)
                       │     │
Voting:                └─Maj─┘       (3-window majority)
                             │
Servo:                       └─Move  (gesture execution)
```

## Interrupt Service Routines

```c
// ISR Priority Configuration
#define EMG_DMA_IRQ_PRIORITY     0  // Highest
#define EMG_DRDY_IRQ_PRIORITY    1
#define ACC_INT_IRQ_PRIORITY     2
#define TIM_UPDATE_IRQ_PRIORITY  3
#define UART_RX_IRQ_PRIORITY     4  // Lowest

// DMA Half/Full Complete ISR
void DMA1_Stream0_IRQHandler(void) {
    if (DMA1->ISR & DMA_ISR_HTIF0) {
        // Half transfer complete
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(emg_half_complete, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    if (DMA1->ISR & DMA_ISR_TCIF0) {
        // Full transfer complete
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(emg_full_complete, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
```

## Power Management

```c
// Power modes for different states
typedef enum {
    POWER_MODE_ACTIVE,        // Full performance
    POWER_MODE_IDLE,          // Reduced sampling
    POWER_MODE_SLEEP,         // Minimal power
} Power_Mode_t;

// Dynamic frequency scaling
void Power_SetMode(Power_Mode_t mode) {
    switch(mode) {
        case POWER_MODE_ACTIVE:
            SystemClock_Config_400MHz();
            EMG_SetSampleRate(1000);
            break;
        case POWER_MODE_IDLE:
            SystemClock_Config_200MHz();
            EMG_SetSampleRate(500);
            break;
        case POWER_MODE_SLEEP:
            SystemClock_Config_64MHz();
            EMG_PowerDown();
            break;
    }
}
```

## Communication Protocol

### UART Debug Interface
```
Commands:
- SYS:INFO?          - Get system information
- SYS:RESET          - Reset system
- EMG:START          - Start acquisition
- EMG:STOP           - Stop acquisition
- EMG:CAL            - Calibrate channels
- ML:TRAIN <class>   - Start training mode
- ML:STATS?          - Get classification stats
- SERVO:TEST <ch>    - Test servo channel
- SERVO:SET <angles> - Set all servo angles
```

### Data Streaming Format
```
Binary packet structure:
┌────┬────┬────────┬─────────┬────┐
│SOF │LEN │ TYPE   │ PAYLOAD │CRC │
│0xAA│ 2B │ 1B     │ N bytes │ 2B │
└────┴────┴────────┴─────────┴────┘

Types:
- 0x01: EMG raw data
- 0x02: Feature vector
- 0x03: Classification result
- 0x04: System status
```

## Error Handling

```c
typedef enum {
    ERROR_NONE = 0,
    ERROR_EMG_TIMEOUT,
    ERROR_EMG_OVERFLOW,
    ERROR_ACC_COMM,
    ERROR_SERVO_FAULT,
    ERROR_ML_MEMORY,
    ERROR_ML_INFERENCE,
} System_Error_t;

typedef struct {
    System_Error_t code;
    uint32_t timestamp;
    uint32_t context;
} Error_Log_t;

// Circular error log
Error_Log_t error_log[32];
uint8_t error_idx = 0;

void Error_Handler(System_Error_t error) {
    // Log error
    error_log[error_idx].code = error;
    error_log[error_idx].timestamp = HAL_GetTick();
    error_idx = (error_idx + 1) & 0x1F;
    
    // Take action based on severity
    switch(error) {
        case ERROR_EMG_TIMEOUT:
        case ERROR_EMG_OVERFLOW:
            EMG_Reset();
            break;
        case ERROR_SERVO_FAULT:
            Servo_EmergencyStop();
            break;
        default:
            // Log only
            break;
    }
}
```

## Performance Metrics

### Latency Budget (Target: <100ms)
```
┌─────────────────────────┬────────┐
│ Component               │ Time   │
├─────────────────────────┼────────┤
│ EMG Acquisition         │ 1ms    │
│ Window Collection       │ 128ms* │
│ STFT Processing         │ 5ms    │
│ Feature Extraction      │ 2ms    │
│ RF Inference            │ 3ms    │
│ Majority Voting         │ 1ms    │
│ Servo Command           │ 1ms    │
├─────────────────────────┼────────┤
│ Total (with overlap)    │ ~75ms  │
└─────────────────────────┴────────┘
* With 50% overlap, effective latency is reduced
```

### Memory Usage Targets
```
┌─────────────────────────┬────────┬────────┐
│ Component               │ RAM    │ Flash  │
├─────────────────────────┼────────┼────────┤
│ EMG Buffers             │ 8KB    │ -      │
│ DSP Working Memory      │ 3KB    │ 2KB    │
│ RF Model + Working      │ 4KB    │ 30KB   │
│ Servo Control           │ 512B   │ 1KB    │
│ System/RTOS             │ 4KB    │ 10KB   │
├─────────────────────────┼────────┼────────┤
│ Total                   │ ~20KB  │ ~43KB  │
└─────────────────────────┴────────┴────────┘
```

## Testing Strategy

### Unit Tests
- DSP functions (FFT, filtering)
- Feature extraction
- RF inference
- Servo control

### Integration Tests
- End-to-end latency
- Memory stress test
- Power consumption
- EMI/RFI immunity

### System Tests
- Gesture recognition accuracy
- User acceptance testing
- Long-term reliability
- Environmental testing

## Future Enhancements

1. **Adaptive Learning**: Online model updates based on user feedback
2. **Multi-Modal Fusion**: Combine EMG with IMU data
3. **Wireless Connectivity**: BLE for configuration and monitoring
4. **Advanced Gestures**: Sequential gesture recognition
5. **Power Optimization**: Predictive power management