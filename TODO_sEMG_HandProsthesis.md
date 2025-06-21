# sEMG Hand Prosthesis Development - Comprehensive To-Do List

## Project Overview
Convert TOF-sensor hand-prosthesis firmware to sEMG-based system with Random Forest classification for Turkish Sign Language recognition.

### System Architecture
- **Input**: 4-channel sEMG + 1-axis accelerometer
- **Processing**: STM32H7 (NUCLEO-H7S3L8)
- **Feature Extraction**: STFT with 3 windows
- **Classification**: Random Forest (≤12 KB RAM, ≤32 KB Flash)
- **Output**: 6 hobby servos via PWM
- **Decision**: Majority voting across windows

### Key Specifications
- **AFE**: ADS1299-compatible, 24-bit, SPI ≤2 Msps
- **EMG Sample Rate**: 1 kHz (configurable)
- **Window**: 256 samples (~256 ms), 50% overlap
- **STFT**: 64-point FFT, Hamming window
- **Accelerometer**: LIS3DH, I2C, 104 Hz
- **Servo Control**: TIM1 PWM @ 50 Hz, 0-180°

---

## Sprint 1: Hardware Design & Validation (2-3 weeks)

### 1.1 Analog Front-End (AFE) Design
- [ ] Design ADS1299-based 4-channel EMG acquisition board
  - [ ] Schematic design with proper EMG filtering (20-500 Hz bandpass)
  - [ ] Implement RLD (Right Leg Drive) for common-mode rejection
  - [ ] Add overvoltage protection for patient safety
  - [ ] Design proper shielding and grounding
- [ ] PCB layout for minimal noise
  - [ ] 4-layer PCB with dedicated ground plane
  - [ ] Minimize trace lengths for analog signals
  - [ ] Proper decoupling capacitors placement
- [ ] Component selection and BOM preparation
  - [ ] Select medical-grade electrodes and connectors
  - [ ] Choose low-noise power regulators
  - [ ] Select protection components (TVS diodes, etc.)

### 1.2 MCU Board Integration
- [ ] Verify NUCLEO-H7S3L8 pinout compatibility
  - [ ] Map SPI pins for ADS1299 (MOSI, MISO, SCK, CS)
  - [ ] Map I2C pins for LIS3DH accelerometer
  - [ ] Map 6 PWM outputs on TIM1 for servos
  - [ ] Reserve UART for debug output
- [ ] Design interface board/shield
  - [ ] Level shifters if needed (3.3V/5V)
  - [ ] Connector layout for modular assembly
  - [ ] Power distribution design

### 1.3 Power System Design
- [ ] Calculate power budget
  - [ ] MCU: ~200mA @ 3.3V
  - [ ] AFE: ~50mA @ 3.3V/5V
  - [ ] Servos: 6 × 500mA @ 5-6V (peak)
  - [ ] Total: ~3.5A peak @ 5V
- [ ] Design power supply
  - [ ] Select appropriate DC-DC converters
  - [ ] Implement separate power rails for analog/digital
  - [ ] Add bulk capacitance for servo transients
- [ ] Battery selection (if portable)
  - [ ] 2S LiPo (7.4V) with appropriate capacity
  - [ ] Battery management system (BMS)

### 1.4 Mechanical Integration Planning
- [ ] Define servo mounting points
- [ ] Design cable management system
- [ ] Plan electrode placement strategy
- [ ] Create 3D models for custom parts

---

## Sprint 2: Firmware Architecture & Drivers (2-3 weeks)

### 2.1 Project Setup
- [ ] Create STM32CubeIDE project for NUCLEO-H7S3L8
- [ ] Configure clock tree (400 MHz max)
- [ ] Set up build system and version control
- [ ] Implement bootloader for OTA updates

### 2.2 Peripheral Drivers

#### 2.2.1 ADS1299 Driver
```c
// Key functions to implement:
- [ ] ADS1299_Init()
- [ ] ADS1299_ConfigureChannel()
- [ ] ADS1299_StartContinuousMode()
- [ ] ADS1299_ReadData() // Via DMA
- [ ] ADS1299_Calibrate()
```

#### 2.2.2 LIS3DH Accelerometer Driver
```c
- [ ] LIS3DH_Init()
- [ ] LIS3DH_ConfigureODR(104Hz)
- [ ] LIS3DH_ReadXYZ()
- [ ] LIS3DH_EnableInterrupt()
```

#### 2.2.3 Servo Control Driver
```c
- [ ] Servo_Init() // Configure TIM1 for 6 channels
- [ ] Servo_SetAngle(channel, angle)
- [ ] Servo_SetMultiple(angles[6])
- [ ] Servo_EnableSmoothTransition()
```

### 2.3 Real-Time Data Acquisition
- [ ] Implement circular buffer for EMG data
  - [ ] Size: 2 × 256 × 4 channels × 4 bytes = 8KB
  - [ ] DMA double-buffering
- [ ] Implement sample-accurate timing
  - [ ] Use hardware timer for 1 kHz sampling
  - [ ] Synchronize EMG and accelerometer data
- [ ] Implement data integrity checks
  - [ ] CRC for SPI communication
  - [ ] Sample drop detection

### 2.4 DSP Pipeline

#### 2.4.1 Preprocessing
```c
- [ ] HighPassFilter_Init() // Remove DC offset
- [ ] NotchFilter_Init() // 50Hz powerline
- [ ] MovingAverage_Init() // Smoothing
```

#### 2.4.2 STFT Implementation
```c
- [ ] STFT_Init()
  - [ ] Allocate FFT buffers (64-point)
  - [ ] Generate Hamming window coefficients
- [ ] STFT_Process()
  - [ ] Apply window
  - [ ] Compute FFT (use CMSIS-DSP)
  - [ ] Calculate magnitude spectrum
- [ ] STFT_ExtractFeatures()
  - [ ] Mean frequency
  - [ ] Spectral power bands
  - [ ] Zero crossing rate
```

### 2.5 Memory Management
- [ ] Implement memory pools for buffers
- [ ] Configure MPU for optimal caching
- [ ] Implement stack overflow detection
- [ ] Profile memory usage

---

## Sprint 3: Machine Learning Pipeline (3-4 weeks)

### 3.1 Feature Engineering

#### 3.1.1 Time-Domain Features
```python
- [ ] RMS (Root Mean Square)
- [ ] MAV (Mean Absolute Value)
- [ ] VAR (Variance)
- [ ] ZC (Zero Crossings)
- [ ] SSC (Slope Sign Changes)
- [ ] WL (Waveform Length)
```

#### 3.1.2 Frequency-Domain Features (from STFT)
```python
- [ ] Mean Power Frequency (MPF)
- [ ] Median Frequency (MDF)
- [ ] Peak Frequency
- [ ] Spectral Moments
- [ ] Power in bands (0-50Hz, 50-150Hz, 150-250Hz, 250-500Hz)
```

#### 3.1.3 Accelerometer Features
```python
- [ ] Mean acceleration per axis
- [ ] Orientation angles
- [ ] Movement detection flag
```

### 3.2 Random Forest Implementation

#### 3.2.1 Model Architecture
```c
typedef struct {
    uint8_t n_trees;        // Start with 10-20 trees
    uint8_t max_depth;      // Limit to 4-6 for memory
    uint8_t n_features;     // ~20-30 features total
    uint8_t n_classes;      // 3 initially, then 29
    TreeNode* trees[MAX_TREES];
} RandomForest;
```

#### 3.2.2 Optimized C Implementation
```c
- [ ] RF_Init()
- [ ] RF_LoadModel() // From flash
- [ ] RF_Predict()
- [ ] RF_GetConfidence()
```

#### 3.2.3 Memory Optimization
- [ ] Use fixed-point arithmetic
- [ ] Implement tree pruning
- [ ] Store in flash with XIP
- [ ] Quantize thresholds to 8-bit

### 3.3 Training Pipeline (Python)

#### 3.3.1 Data Collection Protocol
```python
- [ ] Design data collection GUI
- [ ] Implement synchronized recording
- [ ] Create labeling system for TSL gestures
- [ ] Define train/val/test splits
```

#### 3.3.2 Model Training
```python
- [ ] Implement feature extraction pipeline
- [ ] Train Random Forest with scikit-learn
- [ ] Hyperparameter optimization
- [ ] Cross-validation for 3 classes
```

#### 3.3.3 Model Conversion
```python
- [ ] Export to C header file
- [ ] Implement quantization
- [ ] Verify bit-exact inference
- [ ] Generate memory usage report
```

### 3.4 Majority Voting System
```c
- [ ] Implement sliding window buffer (3 predictions)
- [ ] Calculate mode of predictions
- [ ] Add confidence weighting
- [ ] Implement gesture transition detection
```

---

## Sprint 4: Mechanical Design & Integration (2-3 weeks)

### 4.1 Servo Selection & Testing
- [ ] Select appropriate hobby servos
  - [ ] Torque requirements analysis
  - [ ] Speed requirements (gesture transitions)
  - [ ] Power consumption measurement
- [ ] Create servo characterization profiles
  - [ ] Angle accuracy testing
  - [ ] Response time measurement
  - [ ] Load testing

### 4.2 Hand Mechanism Design
- [ ] Design finger linkage system
  - [ ] 6 servos → 5 fingers + wrist
  - [ ] Implement differential mechanisms
  - [ ] Design tendon routing
- [ ] Create 3D printed prototypes
  - [ ] Iterate on joint design
  - [ ] Optimize for TSL gestures
  - [ ] Ensure reliability

### 4.3 Electrode System
- [ ] Design electrode placement guide
  - [ ] Target flexor/extensor muscle groups
  - [ ] Minimize crosstalk
  - [ ] Ensure repeatability
- [ ] Create electrode mounting system
  - [ ] Adjustable straps
  - [ ] Consistent pressure
  - [ ] Quick-release mechanism

### 4.4 Enclosure Design
- [ ] Design electronics enclosure
  - [ ] EMI shielding considerations
  - [ ] Thermal management
  - [ ] Cable strain relief
- [ ] Create wearable form factor
  - [ ] Weight distribution
  - [ ] Comfort padding
  - [ ] Aesthetic design

---

## Sprint 5: System Integration & Testing (3-4 weeks)

### 5.1 Integration Testing

#### 5.1.1 Hardware Integration
- [ ] Verify all electrical connections
- [ ] Test power supply under load
- [ ] EMC/EMI testing
- [ ] Thermal testing

#### 5.1.2 Firmware Integration
- [ ] End-to-end latency measurement
  - [ ] Target: <100ms from EMG to servo
- [ ] Real-time performance validation
- [ ] Memory leak testing
- [ ] Stress testing (continuous operation)

### 5.2 Performance Optimization

#### 5.2.1 Computational Optimization
- [ ] Profile CPU usage
- [ ] Optimize FFT computation
- [ ] Implement SIMD where applicable
- [ ] Balance accuracy vs. speed

#### 5.2.2 Power Optimization
- [ ] Implement sleep modes
- [ ] Optimize sampling rates
- [ ] Reduce servo holding current
- [ ] Add battery life estimation

### 5.3 Gesture Recognition Testing

#### 5.3.1 Initial 3-Class Validation
- [ ] Define 3 distinct TSL letters
- [ ] Collect training data (10 subjects)
- [ ] Achieve >90% accuracy target
- [ ] Measure confusion matrix

#### 5.3.2 Robustness Testing
- [ ] Test with electrode shift
- [ ] Test with muscle fatigue
- [ ] Test with different users
- [ ] Environmental interference testing

### 5.4 User Interface Development

#### 5.4.1 Configuration Interface
- [ ] UART command interface
- [ ] Bluetooth module integration (optional)
- [ ] Mobile app for configuration
- [ ] Gesture training mode

#### 5.4.2 Feedback Systems
- [ ] LED status indicators
- [ ] Haptic feedback (optional)
- [ ] Audio feedback (optional)
- [ ] Debug data streaming

---

## Sprint 6: Expansion & Deployment (2-3 weeks)

### 6.1 Scaling to 29 Classes

#### 6.1.1 Data Collection Campaign
- [ ] Design comprehensive TSL gesture set
- [ ] Recruit diverse subject pool
- [ ] Implement quality control
- [ ] Create balanced dataset

#### 6.1.2 Model Retraining
- [ ] Expand Random Forest capacity
- [ ] Optimize feature selection
- [ ] Implement class balancing
- [ ] Validate memory constraints

### 6.2 Production Preparation

#### 6.2.1 Documentation
- [ ] User manual
- [ ] Assembly instructions
- [ ] Troubleshooting guide
- [ ] API documentation

#### 6.2.2 Manufacturing Files
- [ ] Gerber files for PCBs
- [ ] STL files for 3D parts
- [ ] BOM with suppliers
- [ ] Assembly drawings

### 6.3 Regulatory Compliance
- [ ] EMC testing (if required)
- [ ] Safety analysis
- [ ] CE marking preparation
- [ ] Open-source license selection

### 6.4 Future Enhancements
- [ ] Online learning capability
- [ ] Multi-user profiles
- [ ] Gesture customization
- [ ] Cloud connectivity

---

## Critical Path Items

1. **AFE Board Design & Fabrication** (Sprint 1) - Blocks all testing
2. **Random Forest Memory Optimization** (Sprint 3) - Must fit constraints
3. **Real-time Performance** (Sprint 5) - Must meet latency target
4. **3-Class Accuracy** (Sprint 5) - Proof of concept validation

## Risk Mitigation

1. **Memory Constraints**: Start with minimal RF model, optimize iteratively
2. **EMG Signal Quality**: Invest in proper AFE design and shielding
3. **Mechanical Reliability**: Extensive testing of servo mechanisms
4. **User Variability**: Collect diverse training data early

## Success Metrics

- [ ] <100ms end-to-end latency
- [ ] >90% accuracy on 3 classes
- [ ] <12KB RAM for RF model
- [ ] <32KB Flash for RF model
- [ ] >4 hours battery life
- [ ] Successful TSL communication demonstration