/**
 * @file emg_acquisition.h
 * @brief EMG data acquisition module using ADS1299
 */

#ifndef EMG_ACQUISITION_H
#define EMG_ACQUISITION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32h7xx_hal.h"

/* Exported types ------------------------------------------------------------*/
typedef struct {
    int32_t data[4];          // 4 channels of 24-bit data
    uint32_t timestamp;       // System tick when sample was acquired
} EMG_Sample_t;

typedef struct {
    EMG_Sample_t samples[256]; // Buffer for DMA transfer
    uint16_t n_samples;       // Number of valid samples
    uint8_t buffer_id;        // 0 or 1 for double buffering
} EMG_Buffer_t;

typedef struct {
    uint32_t sample_rate;     // Sampling rate in Hz
    uint8_t gain;             // PGA gain setting (1, 2, 4, 6, 8, 12, 24)
    uint8_t channels_enabled; // Bit mask for enabled channels
    uint8_t reference_mode;   // 0: External, 1: Internal
} EMG_Config_t;

/* Exported constants --------------------------------------------------------*/
// ADS1299 Commands
#define ADS1299_CMD_WAKEUP    0x02
#define ADS1299_CMD_STANDBY   0x04
#define ADS1299_CMD_RESET     0x06
#define ADS1299_CMD_START     0x08
#define ADS1299_CMD_STOP      0x0A
#define ADS1299_CMD_RDATAC    0x10
#define ADS1299_CMD_SDATAC    0x11
#define ADS1299_CMD_RDATA     0x12

// ADS1299 Registers
#define ADS1299_REG_ID        0x00
#define ADS1299_REG_CONFIG1   0x01
#define ADS1299_REG_CONFIG2   0x02
#define ADS1299_REG_CONFIG3   0x03
#define ADS1299_REG_LOFF      0x04
#define ADS1299_REG_CH1SET    0x05
#define ADS1299_REG_CH2SET    0x06
#define ADS1299_REG_CH3SET    0x07
#define ADS1299_REG_CH4SET    0x08

// Configuration values
#define ADS1299_SAMPLE_RATE_1000HZ  0x86  // fMOD/4096
#define ADS1299_PGA_GAIN_24         0x60  // Gain = 24

/* Exported functions prototypes ---------------------------------------------*/
HAL_StatusTypeDef EMG_Init(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef EMG_Configure(const EMG_Config_t *config);
HAL_StatusTypeDef EMG_StartContinuous(void);
HAL_StatusTypeDef EMG_StopContinuous(void);
HAL_StatusTypeDef EMG_ReadBuffer(EMG_Buffer_t *buffer);
HAL_StatusTypeDef EMG_Calibrate(void);
float EMG_ConvertToVoltage(int32_t raw_value);
HAL_StatusTypeDef EMG_SetGain(uint8_t channel, uint8_t gain);
HAL_StatusTypeDef EMG_EnableChannel(uint8_t channel, bool enable);

/* DMA callback functions */
void EMG_DMA_HalfCompleteCallback(void);
void EMG_DMA_CompleteCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* EMG_ACQUISITION_H */