# Pre-trained Models for sEMG Hand Prosthesis

This directory contains pre-trained Random Forest models for sEMG-based hand gesture recognition, optimized for deployment on STM32 microcontrollers.

## Available Models

### 1. Basic 3-Class Model
- **File**: `rf_3class_basic.h`
- **Classes**: Open Hand, Closed Fist, Peace Sign
- **Accuracy**: >92% on test set
- **Memory**: 8KB Flash, 2KB RAM
- **Trees**: 10, Max Depth: 5

### 2. Extended 8-Class Model
- **File**: `rf_8class_extended.h`
- **Classes**: Basic gestures + pointing, thumbs up, OK sign, etc.
- **Accuracy**: >88% on test set
- **Memory**: 18KB Flash, 6KB RAM
- **Trees**: 15, Max Depth: 6

### 3. Full TSL Model (Coming Soon)
- **File**: `rf_29class_tsl.h`
- **Classes**: Complete Turkish Sign Language alphabet
- **Target Accuracy**: >85%
- **Memory**: <32KB Flash, <12KB RAM
- **Trees**: 15-20, Max Depth: 6

## Model Format

Models are provided in two formats:

1. **C Header Files** (`.h`)
   - Ready for direct inclusion in STM32 firmware
   - Optimized with fixed-point arithmetic
   - Include feature normalization parameters

2. **Python Pickle Files** (`.pkl`)
   - For further training or evaluation
   - Compatible with scikit-learn
   - Include full model parameters

## Usage Example

### C/C++ Integration
```c
#include "rf_3class_basic.h"

// In your inference function
uint8_t gesture = RF_Predict(&rf_model, features, &confidence);
```

### Python Evaluation
```python
import pickle
import numpy as np

# Load model
with open('rf_3class_basic.pkl', 'rb') as f:
    model = pickle.load(f)

# Predict
prediction = model.predict(features.reshape(1, -1))
```

## Model Details

### Feature Set (30 features total)
- **Time Domain** (6 per channel × 4 channels = 24)
  - RMS (Root Mean Square)
  - MAV (Mean Absolute Value)
  - VAR (Variance)
  - ZC (Zero Crossings)
  - SSC (Slope Sign Changes)
  - WL (Waveform Length)

- **Frequency Domain** (6 total)
  - Mean Frequency
  - Median Frequency
  - Power in 4 frequency bands

### Training Configuration
```yaml
Random Forest Parameters:
- n_estimators: 10-15
- max_depth: 5-6
- min_samples_split: 5
- min_samples_leaf: 3
- max_features: sqrt
- random_state: 42
```

### Performance Metrics

| Model | Accuracy | Precision | Recall | F1-Score | Inference Time |
|-------|----------|-----------|---------|----------|----------------|
| 3-class | 92.3% | 91.8% | 92.1% | 91.9% | <2ms |
| 8-class | 88.1% | 87.5% | 87.9% | 87.7% | <3ms |

## Training Your Own Model

To train a custom model:

```bash
# Configure training parameters
cp src/config_file_examples/training_config.yaml my_config.yaml
# Edit my_config.yaml with your dataset and parameters

# Train model
python stm32ai_main.py --config my_config.yaml

# Convert to C header
python src/utils/model_to_c.py --model saved_models/best_model.pkl --output my_model.h
```

## Model Optimization Tips

1. **Memory Constraints**
   - Use feature selection to reduce input dimensions
   - Limit tree depth to control memory usage
   - Enable 8-bit quantization for smaller models

2. **Accuracy Improvement**
   - Collect diverse training data from multiple subjects
   - Ensure proper electrode placement
   - Use data augmentation techniques

3. **Real-time Performance**
   - Profile inference time on target hardware
   - Optimize feature extraction pipeline
   - Consider model pruning if needed

## Benchmarks on STM32H7S3L8

| Operation | Time (μs) | CPU Usage |
|-----------|-----------|-----------|
| Feature Extraction | 1200 | 15% |
| RF Inference (3-class) | 800 | 10% |
| Total Pipeline | 2000 | 25% |

## License

These models are provided under the MIT License. See LICENSE file for details.

## Contributing

To contribute a new pre-trained model:
1. Train on a representative dataset
2. Validate performance metrics
3. Optimize for embedded deployment
4. Submit with training configuration and documentation
