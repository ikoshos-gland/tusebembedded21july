/**
 * @file main.c
 * @brief sEMG Hand Prosthesis - Main Application
 * @author TinyML Consultant
 * @date 2025-01-21
 * 
 * This implements a real-time sEMG-based gesture recognition system
 * for Turkish Sign Language using Random Forest classification.
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "emg_acquisition.h"
#include "dsp_pipeline.h"
#include "random_forest.h"
#include "servo_control.h"
#include "system_monitor.h"

/* Private defines -----------------------------------------------------------*/
#define SYSTEM_CORE_CLOCK   280000000U  // 280 MHz
#define EMG_SAMPLE_RATE     1000U        // 1 kHz
#define WINDOW_SIZE         256U         // 256 samples
#define WINDOW_OVERLAP      128U         // 50% overlap

/* Private variables ---------------------------------------------------------*/
// HAL handles
static SPI_HandleTypeDef hspi1;      // For ADS1299
static I2C_HandleTypeDef hi2c1;      // For LIS3DH
static TIM_HandleTypeDef htim1;      // For servo PWM
static UART_HandleTypeDef huart3;    // For debug

// FreeRTOS handles
static TaskHandle_t emgTaskHandle;
static TaskHandle_t dspTaskHandle;
static TaskHandle_t mlTaskHandle;
static TaskHandle_t servoTaskHandle;
static TaskHandle_t monitorTaskHandle;

static QueueHandle_t emgDataQueue;
static QueueHandle_t featureQueue;
static SemaphoreHandle_t emgReadySem;

// Global system state
static System_State_t system_state = {
    .mode = MODE_IDLE,
    .error_code = ERROR_NONE,
    .battery_voltage = 0.0f,
    .temperature = 25.0f
};

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void SPI1_Init(void);
static void I2C1_Init(void);
static void TIM1_Init(void);
static void UART3_Init(void);
static void Error_Handler(void);

// FreeRTOS tasks
static void EMG_AcquisitionTask(void *pvParameters);
static void DSP_ProcessingTask(void *pvParameters);
static void ML_InferenceTask(void *pvParameters);
static void Servo_ControlTask(void *pvParameters);
static void System_MonitorTask(void *pvParameters);

/* Main function -------------------------------------------------------------*/
int main(void)
{
    // HAL initialization
    HAL_Init();
    
    // Configure system clock (280 MHz)
    SystemClock_Config();
    
    // Initialize peripherals
    GPIO_Init();
    SPI1_Init();
    I2C1_Init();
    TIM1_Init();
    UART3_Init();
    
    // Enable caches for performance
    SCB_EnableICache();
    SCB_EnableDCache();
    
    // Initialize debug console
    printf("\r\n=== sEMG Hand Prosthesis System ===\r\n");
    printf("Firmware Version: 1.0.0\r\n");
    printf("Build Date: " __DATE__ " " __TIME__ "\r\n");
    printf("Core Clock: %lu MHz\r\n", SystemCoreClock / 1000000);
    
    // Initialize hardware modules
    if (EMG_Init(&hspi1) != HAL_OK) {
        printf("ERROR: EMG initialization failed!\r\n");
        Error_Handler();
    }
    
    if (ACC_Init(&hi2c1) != HAL_OK) {
        printf("ERROR: Accelerometer initialization failed!\r\n");
        Error_Handler();
    }
    
    if (Servo_Init(&htim1) != HAL_OK) {
        printf("ERROR: Servo initialization failed!\r\n");
        Error_Handler();
    }
    
    // Load Random Forest model from Flash
    if (RF_LoadModel() != HAL_OK) {
        printf("ERROR: ML model loading failed!\r\n");
        Error_Handler();
    }
    
    printf("Hardware initialization complete.\r\n");
    
    // Create FreeRTOS objects
    emgDataQueue = xQueueCreate(4, sizeof(EMG_Buffer_t));
    featureQueue = xQueueCreate(2, sizeof(Feature_Vector_t));
    emgReadySem = xSemaphoreCreateBinary();
    
    if (emgDataQueue == NULL || featureQueue == NULL || emgReadySem == NULL) {
        printf("ERROR: FreeRTOS object creation failed!\r\n");
        Error_Handler();
    }
    
    // Create tasks with appropriate priorities
    xTaskCreate(EMG_AcquisitionTask, "EMG_Acq", 512, NULL, 5, &emgTaskHandle);
    xTaskCreate(DSP_ProcessingTask, "DSP_Proc", 1024, NULL, 4, &dspTaskHandle);
    xTaskCreate(ML_InferenceTask, "ML_Infer", 768, NULL, 3, &mlTaskHandle);
    xTaskCreate(Servo_ControlTask, "Servo", 512, NULL, 2, &servoTaskHandle);
    xTaskCreate(System_MonitorTask, "Monitor", 512, NULL, 1, &monitorTaskHandle);
    
    printf("Starting FreeRTOS scheduler...\r\n");
    
    // Start scheduler
    vTaskStartScheduler();
    
    // Should never reach here
    while (1) {
        Error_Handler();
    }
}

/* Task Implementations ------------------------------------------------------*/

/**
 * @brief EMG Data Acquisition Task
 * @note Runs at 1 kHz, highest priority
 */
static void EMG_AcquisitionTask(void *pvParameters)
{
    EMG_Buffer_t emg_buffer;
    uint32_t sample_count = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Configure DMA for continuous acquisition
    EMG_StartContinuous();
    
    while (1) {
        // Wait for DMA half/full complete interrupt
        if (xSemaphoreTake(emgReadySem, pdMS_TO_TICKS(10)) == pdTRUE) {
            
            // Read latest samples from DMA buffer
            EMG_ReadBuffer(&emg_buffer);
            
            // Send to DSP task
            if (xQueueSend(emgDataQueue, &emg_buffer, 0) != pdTRUE) {
                // Queue full, data dropped
                system_state.stats.dropped_samples++;
            }
            
            sample_count++;
            
            // Update statistics every second
            if (sample_count >= EMG_SAMPLE_RATE) {
                system_state.stats.emg_sample_rate = sample_count;
                sample_count = 0;
            }
        }
        
        // Maintain 1ms period
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}

/**
 * @brief DSP Processing Task
 * @note Processes windows of EMG data and extracts features
 */
static void DSP_ProcessingTask(void *pvParameters)
{
    EMG_Buffer_t emg_buffer;
    Feature_Vector_t features;
    DSP_Context_t dsp_ctx;
    
    // Initialize DSP context
    DSP_Init(&dsp_ctx);
    
    // Sliding window buffer
    float window_buffer[WINDOW_SIZE][4];  // 4 channels
    uint16_t window_idx = 0;
    
    while (1) {
        // Wait for EMG data
        if (xQueueReceive(emgDataQueue, &emg_buffer, portMAX_DELAY) == pdTRUE) {
            
            // Add samples to sliding window
            for (uint16_t i = 0; i < emg_buffer.n_samples; i++) {
                for (uint8_t ch = 0; ch < 4; ch++) {
                    window_buffer[window_idx][ch] = 
                        EMG_ConvertToVoltage(emg_buffer.samples[i].data[ch]);
                }
                
                window_idx++;
                
                // Process complete window with 50% overlap
                if (window_idx >= WINDOW_SIZE) {
                    uint32_t start_tick = HAL_GetTick();
                    
                    // Extract features
                    DSP_ExtractFeatures(&dsp_ctx, window_buffer, &features);
                    
                    // Send to ML task
                    xQueueSend(featureQueue, &features, 0);
                    
                    // Shift window by 50% (128 samples)
                    memmove(window_buffer, &window_buffer[WINDOW_OVERLAP], 
                           WINDOW_OVERLAP * 4 * sizeof(float));
                    window_idx = WINDOW_OVERLAP;
                    
                    // Update timing statistics
                    system_state.stats.dsp_processing_time = HAL_GetTick() - start_tick;
                }
            }
        }
    }
}

/**
 * @brief Machine Learning Inference Task
 * @note Runs Random Forest classifier and voting
 */
static void ML_InferenceTask(void *pvParameters)
{
    Feature_Vector_t features;
    VotingBuffer_t voting_buffer = {0};
    uint8_t gesture_class;
    uint8_t confidence;
    
    while (1) {
        // Wait for feature vector
        if (xQueueReceive(featureQueue, &features, portMAX_DELAY) == pdTRUE) {
            uint32_t start_tick = HAL_GetTick();
            
            // Run Random Forest inference
            uint8_t prediction = RF_Predict(features.values, &confidence);
            
            // Add to voting buffer
            Voting_AddPrediction(&voting_buffer, prediction, confidence);
            
            // Get majority vote
            uint8_t final_confidence;
            gesture_class = Voting_GetMajority(&voting_buffer, &final_confidence);
            
            // Update gesture if confidence is sufficient
            if (final_confidence > 70) {
                system_state.current_gesture = gesture_class;
                system_state.gesture_confidence = final_confidence;
                
                // Notify servo task
                xTaskNotify(servoTaskHandle, gesture_class, eSetValueWithOverwrite);
            }
            
            // Update statistics
            system_state.stats.ml_inference_time = HAL_GetTick() - start_tick;
            system_state.stats.total_predictions++;
            
            // Debug output
            if (system_state.debug_enabled) {
                printf("Gesture: %d, Confidence: %d%%, Time: %lums\r\n",
                       gesture_class, final_confidence, 
                       system_state.stats.ml_inference_time);
            }
        }
    }
}

/**
 * @brief Servo Control Task
 * @note Updates servo positions based on recognized gestures
 */
static void Servo_ControlTask(void *pvParameters)
{
    uint32_t gesture_class;
    uint8_t current_positions[6] = {90, 90, 90, 90, 90, 90};  // Center position
    uint8_t target_positions[6];
    
    while (1) {
        // Wait for gesture notification
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &gesture_class, pdMS_TO_TICKS(20)) == pdTRUE) {
            
            // Look up target positions for gesture
            Servo_GetGesturePositions(gesture_class, target_positions);
            
            // Smooth transition to new positions
            for (uint8_t step = 0; step < 10; step++) {
                for (uint8_t servo = 0; servo < 6; servo++) {
                    // Linear interpolation
                    int16_t delta = target_positions[servo] - current_positions[servo];
                    current_positions[servo] += delta / 10;
                    
                    // Update servo
                    Servo_SetAngle(servo, current_positions[servo]);
                }
                
                // 20ms per step = 200ms total transition
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            
            // Final position update
            memcpy(current_positions, target_positions, 6);
        } else {
            // No new gesture, maintain current position
            // This also serves as the PWM refresh cycle
            for (uint8_t servo = 0; servo < 6; servo++) {
                Servo_SetAngle(servo, current_positions[servo]);
            }
        }
    }
}

/**
 * @brief System Monitor Task
 * @note Monitors system health and handles commands
 */
static void System_MonitorTask(void *pvParameters)
{
    char rx_buffer[128];
    uint16_t rx_idx = 0;
    
    while (1) {
        // Check for UART commands
        if (UART_Available(&huart3)) {
            char c = UART_GetChar(&huart3);
            
            if (c == '\r' || c == '\n') {
                rx_buffer[rx_idx] = '\0';
                
                // Process command
                if (strncmp(rx_buffer, "SYS:INFO?", 9) == 0) {
                    printf("\r\n=== System Information ===\r\n");
                    printf("Uptime: %lu seconds\r\n", HAL_GetTick() / 1000);
                    printf("EMG Sample Rate: %lu Hz\r\n", system_state.stats.emg_sample_rate);
                    printf("DSP Time: %lu ms\r\n", system_state.stats.dsp_processing_time);
                    printf("ML Time: %lu ms\r\n", system_state.stats.ml_inference_time);
                    printf("Total Predictions: %lu\r\n", system_state.stats.total_predictions);
                    printf("Current Gesture: %d (%d%%)\r\n", 
                           system_state.current_gesture, 
                           system_state.gesture_confidence);
                    printf("Battery: %.2f V\r\n", system_state.battery_voltage);
                    printf("Temperature: %.1f C\r\n", system_state.temperature);
                    printf("Free Heap: %d bytes\r\n", xPortGetFreeHeapSize());
                } 
                else if (strncmp(rx_buffer, "EMG:START", 9) == 0) {
                    system_state.mode = MODE_ACTIVE;
                    printf("EMG acquisition started.\r\n");
                }
                else if (strncmp(rx_buffer, "EMG:STOP", 8) == 0) {
                    system_state.mode = MODE_IDLE;
                    printf("EMG acquisition stopped.\r\n");
                }
                else if (strncmp(rx_buffer, "DEBUG:ON", 8) == 0) {
                    system_state.debug_enabled = true;
                    printf("Debug output enabled.\r\n");
                }
                else if (strncmp(rx_buffer, "DEBUG:OFF", 9) == 0) {
                    system_state.debug_enabled = false;
                    printf("Debug output disabled.\r\n");
                }
                
                rx_idx = 0;
            } else if (rx_idx < sizeof(rx_buffer) - 1) {
                rx_buffer[rx_idx++] = c;
            }
        }
        
        // Update battery voltage (every second)
        static uint32_t last_battery_check = 0;
        if (HAL_GetTick() - last_battery_check > 1000) {
            system_state.battery_voltage = Power_GetBatteryVoltage();
            system_state.temperature = Temp_GetCPUTemperature();
            last_battery_check = HAL_GetTick();
            
            // Check for low battery
            if (system_state.battery_voltage < 6.0f) {
                printf("WARNING: Low battery! %.2f V\r\n", system_state.battery_voltage);
                // Could trigger power saving mode here
            }
        }
        
        // Watchdog reset
        HAL_IWDG_Refresh(&hiwdg);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Peripheral Initialization -------------------------------------------------*/

/**
 * @brief System Clock Configuration
 * @note Configures system clock to 280 MHz using PLL
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // Configure power supply
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    
    // Configure voltage scaling
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    
    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
    
    // Configure HSE and PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;   // HSE = 25MHz / 5 = 5MHz
    RCC_OscInitStruct.PLL.PLLN = 112; // 5MHz * 112 = 560MHz
    RCC_OscInitStruct.PLL.PLLP = 2;   // 560MHz / 2 = 280MHz
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    // Configure clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                  RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV2;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization
 */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Enable GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    
    // Configure LED pins
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Configure ADS1299 control pins
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;  // DRDY, START, RESET
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief SPI1 Initialization for ADS1299
 */
static void SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  // 280MHz/16 = 17.5MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief I2C1 Initialization for LIS3DH
 */
static void I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10909CEC;  // 100kHz @ 280MHz
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief TIM1 Initialization for Servo PWM
 */
static void TIM1_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 279;  // 280MHz / 280 = 1MHz timer clock
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 19999;    // 1MHz / 20000 = 50Hz PWM frequency
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    
    // Configure PWM channels
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;  // 1.5ms = center position
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    // Configure all 6 channels
    for (uint8_t ch = 0; ch < 6; ch++) {
        uint32_t channel = (ch < 4) ? (TIM_CHANNEL_1 << (ch * 4)) : 
                          (ch == 4) ? TIM_CHANNEL_1 : TIM_CHANNEL_2;
        
        if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, channel) != HAL_OK) {
            Error_Handler();
        }
    }
}

/**
 * @brief UART3 Initialization for Debug
 */
static void UART3_Init(void)
{
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief Error Handler
 */
static void Error_Handler(void)
{
    // Disable interrupts
    __disable_irq();
    
    // Turn on error LED
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);  // Red LED
    
    // Log error if possible
    printf("\r\nFATAL ERROR! System halted.\r\n");
    
    // Infinite loop
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        HAL_Delay(100);
    }
}

/* Printf retargeting --------------------------------------------------------*/
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* FreeRTOS Hooks ------------------------------------------------------------*/
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("ERROR: Stack overflow in task %s\r\n", pcTaskName);
    Error_Handler();
}

void vApplicationMallocFailedHook(void)
{
    printf("ERROR: Malloc failed!\r\n");
    Error_Handler();
}