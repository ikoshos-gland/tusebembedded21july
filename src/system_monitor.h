/**
 * @file system_monitor.h
 * @brief System monitoring and diagnostics module
 */

#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
// System health metrics
typedef struct {
    float cpu_usage_percent;      // CPU utilization (0-100%)
    float memory_usage_percent;   // RAM usage (0-100%)
    uint32_t free_heap_bytes;     // Available heap memory
    uint32_t min_free_heap_bytes; // Minimum free heap ever
    float battery_voltage;        // Battery voltage in volts
    float battery_current;        // Battery current in amps
    float temperature_celsius;    // System temperature
    uint32_t uptime_seconds;      // System uptime
} System_Health_t;

// Performance metrics
typedef struct {
    uint32_t emg_samples_processed;
    uint32_t emg_samples_dropped;
    uint32_t gestures_recognized;
    uint32_t inference_count;
    float avg_inference_time_ms;
    float max_inference_time_ms;
    float avg_dsp_time_ms;
    float max_dsp_time_ms;
} Performance_Metrics_t;

// Error log entry
typedef struct {
    uint32_t timestamp;           // When error occurred
    uint8_t error_code;           // Error type
    uint32_t context;             // Additional context
    char message[64];             // Error description
} Error_Log_Entry_t;

// Communication statistics
typedef struct {
    uint32_t uart_bytes_sent;
    uint32_t uart_bytes_received;
    uint32_t uart_errors;
    uint32_t spi_transactions;
    uint32_t spi_errors;
    uint32_t i2c_transactions;
    uint32_t i2c_errors;
} Comm_Stats_t;

/* Exported constants --------------------------------------------------------*/
// Error codes
#define ERROR_CODE_NONE              0x00
#define ERROR_CODE_EMG_TIMEOUT       0x01
#define ERROR_CODE_EMG_OVERFLOW      0x02
#define ERROR_CODE_ACC_COMM_FAIL     0x03
#define ERROR_CODE_SERVO_FAULT       0x04
#define ERROR_CODE_ML_MEMORY         0x05
#define ERROR_CODE_ML_INFERENCE      0x06
#define ERROR_CODE_BATTERY_LOW       0x07
#define ERROR_CODE_OVERTEMPERATURE   0x08
#define ERROR_CODE_WATCHDOG_RESET    0x09
#define ERROR_CODE_STACK_OVERFLOW    0x0A

// Thresholds
#define BATTERY_LOW_THRESHOLD        6.0f   // Volts
#define BATTERY_CRITICAL_THRESHOLD   5.5f   // Volts
#define TEMP_WARNING_THRESHOLD       60.0f  // Celsius
#define TEMP_CRITICAL_THRESHOLD      70.0f  // Celsius
#define CPU_HIGH_USAGE_THRESHOLD     80.0f  // Percent
#define MEMORY_HIGH_USAGE_THRESHOLD  80.0f  // Percent

// Log sizes
#define ERROR_LOG_SIZE               32
#define PERFORMANCE_LOG_SIZE         100

/* Exported functions prototypes ---------------------------------------------*/
// Initialization
void Monitor_Init(void);
void Monitor_Start(void);
void Monitor_Stop(void);

// Health monitoring
void Monitor_UpdateHealth(void);
void Monitor_GetHealth(System_Health_t *health);
float Monitor_GetCPUUsage(void);
float Monitor_GetMemoryUsage(void);
uint32_t Monitor_GetFreeHeap(void);
uint32_t Monitor_GetMinFreeHeap(void);

// Battery monitoring
float Power_GetBatteryVoltage(void);
float Power_GetBatteryCurrent(void);
float Power_GetBatteryPercentage(void);
bool Power_IsBatteryLow(void);
bool Power_IsBatteryCritical(void);

// Temperature monitoring
float Temp_GetCPUTemperature(void);
float Temp_GetBoardTemperature(void);
bool Temp_IsOverheated(void);

// Performance monitoring
void Monitor_RecordEMGSample(bool dropped);
void Monitor_RecordInference(uint32_t time_ms);
void Monitor_RecordDSPProcessing(uint32_t time_ms);
void Monitor_RecordGesture(uint8_t gesture_id);
void Monitor_GetPerformanceMetrics(Performance_Metrics_t *metrics);
void Monitor_ResetPerformanceMetrics(void);

// Error logging
void Monitor_LogError(uint8_t error_code, uint32_t context, const char *message);
uint32_t Monitor_GetErrorCount(void);
bool Monitor_GetLastError(Error_Log_Entry_t *entry);
void Monitor_ClearErrorLog(void);
void Monitor_DumpErrorLog(void);

// Communication monitoring
void Monitor_RecordUARTActivity(uint32_t bytes_sent, uint32_t bytes_received, bool error);
void Monitor_RecordSPIActivity(bool error);
void Monitor_RecordI2CActivity(bool error);
void Monitor_GetCommStats(Comm_Stats_t *stats);

// System commands
void Monitor_ProcessCommand(const char *command);
void Monitor_PrintSystemInfo(void);
void Monitor_PrintHealthReport(void);
void Monitor_PrintPerformanceReport(void);

// Watchdog
void Monitor_FeedWatchdog(void);
void Monitor_EnableWatchdog(uint32_t timeout_ms);
void Monitor_DisableWatchdog(void);

// Data logging
void Monitor_EnableDataLogging(bool enable);
bool Monitor_IsDataLoggingEnabled(void);
void Monitor_LogData(const char *format, ...);

// UART helper functions
bool UART_Available(void *huart);
char UART_GetChar(void *huart);
void UART_SendString(void *huart, const char *str);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_MONITOR_H */