# sEMG Dataset for Hand Prosthesis Control

This directory contains datasets for training the Random Forest classifier for sEMG-based hand gesture recognition.

## Dataset Structure

| Dataset | Sensors | Classes | Gestures | Sample Rate | Duration |
|---------|---------|---------|----------|-------------|----------|
| TSL_3class_dataset | 4ch EMG + 1-axis Acc | 3 | Open Hand, Closed Fist, Peace Sign | 1 kHz | TBD |
| TSL_29class_dataset | 4ch EMG + 1-axis Acc | 29 | Full Turkish Sign Language Alphabet | 1 kHz | TBD |

## Create Your Own EMG Dataset

### 1. Hardware Setup

Ensure your EMG acquisition system is properly configured:
- ADS1299 connected via SPI to STM32H7
- 4 differential EMG channels with proper electrode placement
- LIS3DH accelerometer for motion detection
- Proper grounding and shielding

### 2. Electrode Placement

Follow this standardized electrode placement for consistency:

```
Forearm Cross-Section (Looking from elbow toward hand):

        Dorsal (Top)
           CH3
    ┌──────┴──────┐
CH4 │             │ CH2    Legend:
    │   Forearm   │        CH1: Flexor Carpi Radialis
    │             │        CH2: Flexor Digitorum
    └──────┬──────┘        CH3: Extensor Digitorum
           CH1             CH4: Extensor Carpi Ulnaris
       Volar (Palm)        REF: Elbow (bony prominence)
```

### 3. Data Collection Protocol

#### Using Python Collection Script

```bash
# Basic collection
python collect_emg_data.py --port COM3 --gesture "open_hand" --duration 60 --subject "user01"

# Advanced collection with parameters
python collect_emg_data.py \
    --port COM3 \
    --gesture "peace_sign" \
    --duration 120 \
    --subject "user01" \
    --session 1 \
    --sample_rate 1000 \
    --channels 4
```

#### Using UART Commands

Connect via serial terminal (115200 baud):
```
EMG:START              # Start acquisition
ML:TRAIN open_hand     # Start training mode for specific gesture
ML:TRAIN stop          # Stop training mode
EMG:STOP               # Stop acquisition
```

### 4. Dataset Organization

The collected data should be organized as follows:

```
datasets/
├── TSL_3class_dataset/
│   ├── open_hand/
│   │   ├── subject01_session01_001.npz
│   │   ├── subject01_session01_002.npz
│   │   └── ...
│   ├── closed_fist/
│   │   ├── subject01_session01_001.npz
│   │   └── ...
│   └── peace_sign/
│       ├── subject01_session01_001.npz
│       └── ...
└── TSL_29class_dataset/
    ├── A/
    ├── B/
    ├── C/
    └── ... (all 29 TSL letters)
```

### 5. Data File Format

Each `.npz` file contains:
```python
{
    'emg_data': np.array,      # Shape: (n_samples, 4) - 4 EMG channels
    'acc_data': np.array,      # Shape: (n_samples//10,) - 1-axis accelerometer
    'timestamp': np.array,     # Shape: (n_samples,) - Sample timestamps
    'label': str,              # Gesture label
    'subject': str,            # Subject identifier
    'session': int,            # Session number
    'metadata': dict           # Additional metadata
}
```

## Data Collection Guidelines

### Subject Preparation
1. Clean skin with alcohol wipes
2. Apply conductive gel to electrodes
3. Ensure firm electrode contact
4. Check signal quality before recording

### Recording Protocol
1. **Rest Position**: Start with arm relaxed
2. **Gesture Formation**: Form gesture smoothly
3. **Hold Duration**: Maintain gesture for 2-3 seconds
4. **Return to Rest**: Relax arm between gestures
5. **Repetitions**: 20-30 repetitions per gesture per session

### Quality Control
- Monitor real-time signal quality
- Check for electrode disconnection
- Avoid excessive movement artifacts
- Maintain consistent arm position
- Record multiple sessions over different days

## Data Preprocessing

### Basic Preprocessing Pipeline

```python
import numpy as np
from scipy import signal

def preprocess_emg(raw_data, fs=1000):
    """
    Basic EMG preprocessing
    """
    # Remove DC offset
    data = raw_data - np.mean(raw_data, axis=0)
    
    # Bandpass filter (20-500 Hz)
    b, a = signal.butter(4, [20, 500], btype='band', fs=fs)
    filtered = signal.filtfilt(b, a, data, axis=0)
    
    # Notch filter for 50Hz powerline
    b_notch, a_notch = signal.iirnotch(50, 30, fs)
    filtered = signal.filtfilt(b_notch, a_notch, filtered, axis=0)
    
    return filtered
```

### Window Extraction

```python
def extract_windows(data, window_size=256, overlap=0.5):
    """
    Extract overlapping windows from continuous data
    """
    step = int(window_size * (1 - overlap))
    windows = []
    
    for i in range(0, len(data) - window_size + 1, step):
        windows.append(data[i:i + window_size])
    
    return np.array(windows)
```

## Dataset Statistics

### Recommended Dataset Size
- **Per Gesture**: 500-1000 windows
- **Per Subject**: 5-10 subjects for generalization
- **Total Windows**: ~15,000 for 3-class, ~150,000 for 29-class

### Data Augmentation
Consider these augmentation techniques:
- Gaussian noise addition (SNR 20-40 dB)
- Time shifting (±10 samples)
- Amplitude scaling (0.8-1.2x)
- Channel permutation (for robustness)

## Troubleshooting

### Common Issues
1. **Poor Signal Quality**
   - Check electrode impedance (<5kΩ)
   - Ensure proper skin preparation
   - Verify cable connections

2. **Motion Artifacts**
   - Use accelerometer to detect movement
   - Instruct subjects to minimize arm movement
   - Consider motion artifact removal algorithms

3. **Class Imbalance**
   - Ensure equal samples per class
   - Use stratified sampling
   - Apply class weights during training

## References

1. EMG Signal Processing: Merletti & Parker, "Electromyography: Physiology, Engineering, and Non-Invasive Applications"
2. Feature Extraction: Phinyomark et al., "Feature reduction and selection for EMG signal classification"
3. Turkish Sign Language: Turkish Language Association Sign Language Dictionary
