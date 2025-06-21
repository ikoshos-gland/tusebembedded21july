/**
 * @file dsp_pipeline.h
 * @brief Digital Signal Processing pipeline for EMG feature extraction
 */

#ifndef DSP_PIPELINE_H
#define DSP_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
// Feature vector containing all extracted features
typedef struct {
    float values[30];         // Up to 30 features
    uint8_t n_features;       // Actual number of features
    uint32_t timestamp;       // When features were extracted
} Feature_Vector_t;

// DSP context for processing
typedef struct {
    // FFT buffers
    float fft_input[128];     // Real + imaginary interleaved
    float fft_output[128];    
    float magnitude[64];      // Magnitude spectrum
    
    // Window coefficients
    float hamming_window[64];
    
    // Filter states
    float hp_filter_state[4][2];  // High-pass filter state for 4 channels
    float notch_filter_state[4][4]; // 50Hz notch filter state
    
    // Feature extraction parameters
    uint16_t window_size;
    uint16_t fft_size;
    float sample_rate;
} DSP_Context_t;

// Time-domain features
typedef struct {
    float rms;                // Root Mean Square
    float mav;                // Mean Absolute Value
    float var;                // Variance
    uint16_t zc;              // Zero Crossings
    uint16_t ssc;             // Slope Sign Changes
    float wl;                 // Waveform Length
} TimeDomainFeatures_t;

// Frequency-domain features
typedef struct {
    float mean_freq;          // Mean Power Frequency
    float median_freq;        // Median Frequency
    float peak_freq;          // Peak Frequency
    float total_power;        // Total spectral power
    float band_power[4];      // Power in frequency bands
} FrequencyDomainFeatures_t;

/* Exported constants --------------------------------------------------------*/
#define DSP_WINDOW_SIZE       256
#define DSP_FFT_SIZE          64
#define DSP_OVERLAP_SIZE      128
#define DSP_SAMPLE_RATE       1000.0f

// Frequency bands for power calculation (Hz)
#define BAND1_LOW   0.0f
#define BAND1_HIGH  50.0f
#define BAND2_LOW   50.0f
#define BAND2_HIGH  150.0f
#define BAND3_LOW   150.0f
#define BAND3_HIGH  250.0f
#define BAND4_LOW   250.0f
#define BAND4_HIGH  500.0f

/* Exported functions prototypes ---------------------------------------------*/
// Initialization
HAL_StatusTypeDef DSP_Init(DSP_Context_t *ctx);
HAL_StatusTypeDef DSP_Reset(DSP_Context_t *ctx);

// Main processing function
HAL_StatusTypeDef DSP_ExtractFeatures(DSP_Context_t *ctx, 
                                     float window_data[][4], 
                                     Feature_Vector_t *features);

// Preprocessing functions
void DSP_RemoveDCOffset(float *data, uint16_t length);
void DSP_ApplyHighPassFilter(DSP_Context_t *ctx, float *data, uint8_t channel, uint16_t length);
void DSP_ApplyNotchFilter(DSP_Context_t *ctx, float *data, uint8_t channel, uint16_t length);
void DSP_ApplyBandpassFilter(float *data, uint16_t length, float low_freq, float high_freq, float sample_rate);

// Window functions
void DSP_GenerateHammingWindow(float *window, uint16_t size);
void DSP_ApplyWindow(const float *data, const float *window, float *output, uint16_t size);

// FFT functions
void DSP_ComputeFFT(float *input, float *output, uint16_t size);
void DSP_ComputeMagnitudeSpectrum(const float *complex_data, float *magnitude, uint16_t size);

// Time-domain feature extraction
void DSP_ExtractTimeDomainFeatures(const float *data, uint16_t length, TimeDomainFeatures_t *features);
float DSP_CalculateRMS(const float *data, uint16_t length);
float DSP_CalculateMAV(const float *data, uint16_t length);
float DSP_CalculateVariance(const float *data, uint16_t length);
uint16_t DSP_CountZeroCrossings(const float *data, uint16_t length, float threshold);
uint16_t DSP_CountSlopeSignChanges(const float *data, uint16_t length);
float DSP_CalculateWaveformLength(const float *data, uint16_t length);

// Frequency-domain feature extraction
void DSP_ExtractFrequencyDomainFeatures(const float *magnitude, uint16_t size, 
                                       float sample_rate, FrequencyDomainFeatures_t *features);
float DSP_CalculateMeanFrequency(const float *magnitude, uint16_t size, float freq_resolution);
float DSP_CalculateMedianFrequency(const float *magnitude, uint16_t size, float freq_resolution);
float DSP_CalculateBandPower(const float *magnitude, uint16_t size, 
                            float freq_resolution, float low_freq, float high_freq);

// Utility functions
void DSP_NormalizeFeatures(Feature_Vector_t *features);
float DSP_GetFrequencyResolution(float sample_rate, uint16_t fft_size);

#ifdef __cplusplus
}
#endif

#endif /* DSP_PIPELINE_H */