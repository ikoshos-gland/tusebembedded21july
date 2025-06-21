# sEMG Hand Prosthesis with Random Forest Classification

## Overview

This project implements a real-time surface EMG (sEMG) based gesture recognition system for Turkish Sign Language (TSL) using an STM32H7S3L8 microcontroller. The system classifies hand gestures from 4-channel EMG signals and controls 6 hobby servos to replicate the gestures.

## Key Features

- **Real-time Processing**: <100ms end-to-end latency from EMG signal to servo movement
- **Machine Learning**: Random Forest classifier optimized for embedded systems (<12KB RAM, <32KB Flash)
- **Signal Processing**: STFT-based feature extraction with 3-window majority voting
- **Modular Design**: Expandable from 3 to 29 Turkish Sign Language letters
- **Hardware**: Custom EMG acquisition board with ADS1299 ADC and STM32H7 MCU

## Directory Structure

```
├── TODO_sEMG_HandProsthesis.md      # Comprehensive development roadmap
├── firmware_architecture.md          # Detailed firmware design
├── hardware_design_guide.md          # Hardware schematics and PCB guidelines
├── random_forest_embedded.md         # ML implementation for embedded systems
├── PROJECT_SUMMARY.md               # Executive overview
├── datasets/                        # EMG dataset collection and management
├── deployment/                      # Deployment tools and scripts
├── pretrained_models/              # Pre-trained Random Forest models
└── src/                            # Source code and utilities
    ├── main.c                      # Main application
    ├── *.h                         # Module headers
    ├── training/                   # Model training scripts
    ├── evaluation/                 # Model evaluation tools
    └── utils/                      # Utility functions
```

## Quick Start

### 1. Hardware Setup
- Build EMG acquisition board according to `hardware_design_guide.md`
- Connect to NUCLEO-H7S3L8 development board
- Wire 6 hobby servos to PWM outputs

### 2. Firmware Development
```bash
# Clone the repository
git clone [<repository-url>](https://github.com/ikoshos-gland/tusebembedded21july)
cd tusebembedded21july

# Open in STM32CubeIDE
# Build and flash firmware
```

### 3. Data Collection
```bash
# Collect EMG data for training
python src/utils/collect_emg_data.py --port COM3 --gesture "open_hand" --duration 60
```

### 4. Model Training
```bash
# Train Random Forest model
python src/training/train_rf.py --config src/config_file_examples/training_config.yaml
```

### 5. Deployment
```bash
# Deploy model to STM32
python deployment/deploy.py --model pretrained_models/rf_3class.h --port COM3
```

## System Specifications

### Hardware
- **MCU**: STM32H7S3L8 (280 MHz Cortex-M7)
- **EMG ADC**: ADS1299-compatible, 24-bit, 4 channels
- **Accelerometer**: LIS3DH, 3-axis, I2C
- **Servos**: 6x hobby servos via TIM1 PWM
- **Power**: 7.4V LiPo battery

### Signal Processing
- **Sample Rate**: 1 kHz (configurable)
- **Window**: 256 samples with 50% overlap
- **STFT**: 64-point FFT with Hamming window
- **Features**: ~30 time/frequency domain features

### Machine Learning
- **Algorithm**: Random Forest
- **Trees**: 10-15 with max depth 6
- **Classes**: 3 initially (expandable to 29)
- **Accuracy Target**: >90%

## Development Workflow

1. **Phase 1**: Hardware assembly and validation
2. **Phase 2**: Firmware implementation and drivers
3. **Phase 3**: Signal processing pipeline
4. **Phase 4**: Machine learning integration
5. **Phase 5**: System testing and optimization
6. **Phase 6**: Expansion to full TSL alphabet

## Documentation

- [Development Roadmap](TODO_sEMG_HandProsthesis.md)
- [Firmware Architecture](firmware_architecture.md)
- [Hardware Design Guide](hardware_design_guide.md)
- [ML Implementation](random_forest_embedded.md)
- [Project Summary](PROJECT_SUMMARY.md)


## Acknowledgments

- STMicroelectronics for STM32 ecosystem
- Texas Instruments for ADS1299 reference design
- scikit-learn for Random Forest implementation
