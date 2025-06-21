# sEMG Hand Prosthesis Project Summary

## Overview

This project implements a real-time sEMG-based gesture recognition system for Turkish Sign Language using an STM32H7S3L8 microcontroller. The system uses 4-channel surface EMG signals and a 1-axis accelerometer to classify hand gestures through a Random Forest classifier, controlling 6 hobby servos to replicate the gestures.

## Project Structure

```
stm32ai-modelzoo-services/hand_posture/
├── TODO_sEMG_HandProsthesis.md      # Comprehensive development roadmap
├── firmware_architecture.md          # Detailed firmware design
├── hardware_design_guide.md          # Hardware schematics and PCB guidelines
├── random_forest_embedded.md         # ML implementation for embedded systems
├── PROJECT_SUMMARY.md               # This file
└── src/
    ├── main.c                       # Main application entry point
    ├── main.h                       # Main application header
    ├── emg_acquisition.h            # EMG data acquisition interface
    ├── dsp_pipeline.h               # Signal processing pipeline
    ├── random_forest.h              # Random Forest classifier
    ├── servo_control.h              # Servo motor control
    └── system_monitor.h             # System health monitoring
```

## Key Specifications

### Hardware
- **MCU**: STM32H7S3L8 (NUCLEO-H7S3L8 board)
- **EMG ADC**: ADS1299-compatible, 24-bit, 4 channels
- **Accelerometer**: LIS3DH, 3-axis, I2C interface
- **Servos**: 6 hobby servos via TIM1 PWM @ 50 Hz
- **Power**: 7.4V LiPo battery with buck/LDO regulation

### Signal Processing
- **Sample Rate**: 1 kHz (parameterizable)
- **Window Size**: 256 samples (~256 ms)
- **Overlap**: 50% (128 samples)
- **STFT**: 64-point FFT with Hamming window
- **Features**: ~30 time and frequency domain features

### Machine Learning
- **Algorithm**: Random Forest classifier
- **Memory**: <12 KB RAM, <32 KB Flash
- **Trees**: 10-15 trees, max depth 6
- **Classes**: 3 initially (expandable to 29 TSL letters)
- **Voting**: 3-window majority voting for stability

### Performance Targets
- **End-to-end latency**: <100 ms
- **Classification accuracy**: >90% (3 classes)
- **Power consumption**: <3W average
- **Battery life**: >4 hours continuous use

## Development Phases

### Phase 1: Hardware Foundation (Current)
- Design and fabricate EMG acquisition board
- Integrate with NUCLEO-H7S3L8
- Implement basic peripheral drivers
- Verify signal quality

### Phase 2: Signal Processing
- Implement real-time DSP pipeline
- Extract time/frequency features
- Optimize for ARM Cortex-M7
- Validate feature quality

### Phase 3: Machine Learning
- Train Random Forest model in Python
- Convert to embedded C code
- Implement fixed-point inference
- Optimize memory usage

### Phase 4: System Integration
- Integrate all components
- Implement servo control
- Add safety features
- Performance optimization

### Phase 5: Testing & Validation
- Accuracy testing with multiple users
- Reliability and robustness testing
- Power consumption optimization
- User interface development

### Phase 6: Expansion
- Scale from 3 to 29 TSL gestures
- Add online learning capability
- Implement wireless connectivity
- Production preparation

## Quick Start Guide

### 1. Hardware Setup
```bash
# Connect components according to hardware_design_guide.md
# - ADS1299 to SPI1 (PA4-7)
# - LIS3DH to I2C1 (PB8-9)
# - Servos to TIM1 channels
# - Debug UART to USART3
```

### 2. Firmware Build
```bash
# Using STM32CubeIDE
1. Import project
2. Configure for NUCLEO-H7S3L8
3. Build with Release configuration
4. Flash to target
```

### 3. Initial Testing
```bash
# Via UART terminal (115200 baud)
SYS:INFO?      # Get system information
EMG:START      # Start acquisition
EMG:CAL        # Calibrate channels
DEBUG:ON       # Enable debug output
```

### 4. Data Collection
```python
# Python script for training data
python collect_emg_data.py --port COM3 --duration 60 --gesture "open_hand"
```

### 5. Model Training
```python
# Train Random Forest model
python train_rf_model.py --data collected_data/ --output rf_model.h
```

## Key Implementation Files

### Core System (main.c)
- FreeRTOS task scheduling
- Peripheral initialization
- System state management
- Error handling

### EMG Acquisition (emg_acquisition.h)
- ADS1299 SPI driver
- DMA double buffering
- 24-bit to float conversion
- Channel configuration

### DSP Pipeline (dsp_pipeline.h)
- Bandpass filtering (20-500 Hz)
- STFT computation
- Feature extraction
- Real-time processing

### Random Forest (random_forest.h)
- Fixed-point inference
- Memory-optimized trees
- Majority voting
- <3ms inference time

### Servo Control (servo_control.h)
- PWM generation
- Gesture mapping
- Smooth transitions
- Safety limits

## Memory Budget

### RAM Usage (Target: 64KB total)
```
EMG Buffers:        8 KB  (double buffering)
DSP Working:        3 KB  (FFT, filters)
RF Inference:       4 KB  (model + working)
FreeRTOS:          4 KB  (tasks, queues)
Stack:             4 KB  (5 tasks)
Heap:              8 KB  (dynamic allocation)
System:            5 KB  (peripherals, misc)
-----------------------
Total:            36 KB  (56% utilization)
```

### Flash Usage (Target: 512KB total)
```
Application:      80 KB  (core firmware)
RF Model:         32 KB  (15 trees)
DSP Tables:       10 KB  (FFT, windows)
Bootloader:       16 KB  (OTA updates)
Calibration:       4 KB  (user data)
-----------------------
Total:           142 KB  (28% utilization)
```

## Testing Checklist

- [ ] Hardware smoke test (power, connections)
- [ ] EMG signal acquisition (noise <2µV RMS)
- [ ] Accelerometer communication
- [ ] Servo movement test
- [ ] DSP pipeline validation
- [ ] RF model inference accuracy
- [ ] Real-time performance (<100ms)
- [ ] Power consumption measurement
- [ ] EMI/EMC compliance
- [ ] User acceptance testing

## Known Issues & Limitations

1. **Current Implementation**
   - Headers only (no .c files yet)
   - Placeholder for STM32 HAL
   - No actual RF model data

2. **Hardware Considerations**
   - EMG electrode placement critical
   - Requires proper skin preparation
   - Sensitive to motion artifacts

3. **Software Limitations**
   - Fixed 3-class model initially
   - No online learning yet
   - Limited to predefined gestures

## Next Steps

1. **Immediate** (Week 1-2)
   - Complete hardware assembly
   - Implement core drivers
   - Verify signal acquisition

2. **Short-term** (Week 3-4)
   - Complete DSP pipeline
   - Collect training data
   - Train initial RF model

3. **Medium-term** (Month 2)
   - System integration
   - Performance optimization
   - Reliability testing

4. **Long-term** (Month 3+)
   - Expand to 29 gestures
   - Add wireless connectivity
   - Production preparation

## Resources

### Documentation
- [STM32H7 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0433)
- [ADS1299 Datasheet](https://www.ti.com/lit/ds/symlink/ads1299.pdf)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation)

### Tools
- STM32CubeIDE
- Python 3.8+ with scikit-learn
- Serial terminal (Tera Term, PuTTY)
- Oscilloscope for signal verification

### Support
- GitHub Issues for bug reports
- Email: embedded@example.com
- Discord: #semg-prosthesis

## License

This project is released under the MIT License. See LICENSE file for details.

## Acknowledgments

- STMicroelectronics for STM32 ecosystem
- Texas Instruments for ADS1299 reference design
- scikit-learn team for Random Forest implementation
- FreeRTOS community for RTOS support

---

**Project Status**: 🟡 In Development (Phase 1: Hardware Foundation)

Last Updated: January 21, 2025