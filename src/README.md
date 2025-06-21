# sEMG Hand Prosthesis - Source Code

This directory contains the source code, training scripts, and utilities for the sEMG-based hand prosthesis control system using Random Forest classification.

## Directory Structure

```
src/
├── main.c                      # Main firmware application
├── main.h                      # Main application header
├── emg_acquisition.h           # EMG data acquisition interface
├── dsp_pipeline.h              # Signal processing pipeline
├── random_forest.h             # Random Forest classifier
├── servo_control.h             # Servo motor control
├── system_monitor.h            # System health monitoring
├── benchmarking/               # Model benchmarking tools
├── config_file_examples/       # Example configuration files
├── data_augmentation/          # Data augmentation utilities
├── evaluation/                 # Model evaluation scripts
├── models/                     # Model architectures
├── preprocessing/              # Data preprocessing tools
├── training/                   # Training scripts
└── utils/                      # Utility functions
```

## Table of Contents

1. [sEMG Model Training](#1)
2. [Configuration Guide](#2)
   - [2.1 Operation Modes](#2-1)
   - [2.2 General Settings](#2-2)
   - [2.3 Dataset Configuration](#2-3)
   - [2.4 EMG Preprocessing](#2-4)
   - [2.5 Data Augmentation](#2-5)
   - [2.6 Training Parameters](#2-6)
   - [2.7 Model Benchmarking](#2-7)
   - [2.8 Deployment](#2-8)
3. [Results Visualization](#3)
   - [3.1 Training Outputs](#3-1)
   - [3.2 TensorBoard](#3-2)
   - [3.3 MLFlow](#3-3)

<details open><summary><a href="#1"><b>1. sEMG Model Training</b></a></summary><a id="1"></a>

The sEMG hand prosthesis system uses a Random Forest classifier to recognize hand gestures from 4-channel surface EMG signals. The training pipeline includes:

1. **Data Collection**: EMG signals recorded at 1 kHz with proper electrode placement
2. **Preprocessing**: Bandpass filtering (20-500 Hz), notch filtering (50 Hz)
3. **Feature Extraction**: Time and frequency domain features from STFT
4. **Model Training**: Random Forest with memory optimization for embedded deployment
5. **Validation**: Cross-validation and real-time testing

The EMG datasets should be organized as follows:

```bash
dataset_directory/
├── open_hand/
│   ├── subject01_session01_001.npz
│   ├── subject01_session01_002.npz
│   └── ...
├── closed_fist/
│   ├── subject01_session01_001.npz
│   └── ...
└── peace_sign/
    ├── subject01_session01_001.npz
    └── ...
```

Each `.npz` file contains:
- `emg_data`: 4-channel EMG signals (shape: [n_samples, 4])
- `acc_data`: 1-axis accelerometer data (shape: [n_samples//10])
- `timestamp`: Sample timestamps
- `label`: Gesture label
- `metadata`: Recording information

</details>

<details open><summary><a href="#2"><b>2. Configuration Guide</b></a></summary><a id="2"></a>

The system configuration is managed through YAML files. Example configurations are provided in the `config_file_examples/` directory.

<ul><details open><summary><a href="#2-1">2.1 Operation Modes</a></summary><a id="2-1"></a>

The `operation_mode` attribute specifies the task to execute:

| operation_mode | Description |
|:--------------|:------------|
| `training` | Train Random Forest model from EMG data |
| `evaluation` | Evaluate model accuracy on test dataset |
| `benchmarking` | Benchmark model on STM32 target |
| `deployment` | Deploy model to STM32 board |

Example:
```yaml
operation_mode: training
```

</details></ul>

<ul><details open><summary><a href="#2-2">2.2 General Settings</a></summary><a id="2-2"></a>

```yaml
general:
  project_name: semg_prosthesis
  logs_dir: logs
  saved_models_dir: saved_models
  model_path: null  # Path to existing model (optional)
  global_seed: 123
  deterministic_ops: False
  display_figures: True
  gpu_memory_limit: 8
```

Key parameters:
- `project_name`: Identifier for the experiment
- `global_seed`: Random seed for reproducibility
- `model_path`: Use existing model instead of training new one

</details></ul>

<ul><details open><summary><a href="#2-3">2.3 Dataset Configuration</a></summary><a id="2-3"></a>

```yaml
dataset:
  name: TSL_3class_dataset
  class_names: [open_hand, closed_fist, peace_sign]
  training_path: ../datasets/TSL_3class_dataset
  validation_split: 0.2
  test_path: null  # Optional separate test set
  
  # EMG-specific parameters
  sampling_rate: 1000  # Hz
  channels: 4
  window_size: 256
  overlap: 0.5
```

The dataset configuration includes:
- Class definitions for gestures
- Data paths and splits
- EMG signal parameters

</details></ul>

<ul><details open><summary><a href="#2-4">2.4 EMG Preprocessing</a></summary><a id="2-4"></a>

```yaml
preprocessing:
  # Filtering parameters
  bandpass:
    low_freq: 20    # Hz
    high_freq: 500  # Hz
    order: 4
  
  notch_filter:
    freq: 50        # Hz (powerline)
    quality: 30
  
  # Feature extraction
  features:
    time_domain:
      - rms
      - mav
      - var
      - zc
      - ssc
      - wl
    frequency_domain:
      - mean_freq
      - median_freq
      - band_power
  
  # STFT parameters
  stft:
    window_size: 64
    window_type: hamming
    overlap: 0.5
```

</details></ul>

<ul><details open><summary><a href="#2-5">2.5 Data Augmentation</a></summary><a id="2-5"></a>

Data augmentation helps improve model robustness:

```yaml
data_augmentation:
  gaussian_noise:
    snr_db: [20, 40]  # Signal-to-noise ratio range
  
  time_shift:
    max_shift: 10     # samples
  
  amplitude_scaling:
    scale_range: [0.8, 1.2]
  
  channel_permutation:
    probability: 0.1
```

</details></ul>

<ul><details open><summary><a href="#2-6">2.6 Training Parameters</a></summary><a id="2-6"></a>

```yaml
training:
  model:
    type: random_forest
    n_estimators: 15
    max_depth: 6
    max_features: sqrt
    min_samples_split: 5
    min_samples_leaf: 3
  
  # For neural network alternative
  # model:
  #   type: cnn1d
  #   layers: [64, 128, 64]
  #   dropout: 0.2
  
  validation:
    k_fold: 5
    stratified: True
  
  optimization:
    quantization: True
    pruning_threshold: 0.01
  
  save_format: [h5, c_header]
```

</details></ul>

<ul><details open><summary><a href="#2-7">2.7 Model Benchmarking</a></summary><a id="2-7"></a>

```yaml
tools:
  stedgeai:
    version: 10.0.0
    optimization: balanced
    on_cloud: True
    path_to_stedgeai: C:/STM32Cube/X-CUBE-AI/stedgeai.exe

benchmarking:
  board: NUCLEO-H743ZI2  # Target board
  metrics:
    - inference_time
    - ram_usage
    - flash_usage
    - accuracy
```

</details></ul>

<ul><details open><summary><a href="#2-8">2.8 Deployment</a></summary><a id="2-8"></a>

```yaml
deployment:
  target_board: NUCLEO-H7S3L8
  c_project_path: ../firmware/
  ide: STM32CubeIDE
  
  hardware_config:
    emg_adc: ADS1299
    accelerometer: LIS3DH
    servos: 6
    
  communication:
    uart_baudrate: 115200
    spi_speed: 10000000  # 10 MHz
    i2c_speed: 400000    # 400 kHz
```

</details></ul>

</details>

<details open><summary><a href="#3"><b>3. Results Visualization</b></a></summary><a id="3"></a>

<ul><details open><summary><a href="#3-1">3.1 Training Outputs</a></summary><a id="3-1"></a>

Training results are saved in `experiments_outputs/<timestamp>/`:

```
experiments_outputs/
└── 2025_01_21_10_30_45/
    ├── saved_models/
    │   ├── rf_model.pkl          # Trained Random Forest
    │   ├── rf_model.h             # C header file
    │   └── model_stats.json      # Performance metrics
    ├── logs/
    │   ├── training.log          # Training log
    │   └── tensorboard/          # TensorBoard files
    └── plots/
        ├── confusion_matrix.png   # Classification results
        ├── feature_importance.png # RF feature importance
        └── roc_curves.png        # ROC curves per class
```

Example outputs:
- **Confusion Matrix**: Shows classification accuracy per gesture
- **Feature Importance**: Identifies most discriminative EMG features
- **Training Curves**: Accuracy and loss over training iterations

</details></ul>

<ul><details open><summary><a href="#3-2">3.2 TensorBoard</a></summary><a id="3-2"></a>

To visualize training progress:

```bash
cd experiments_outputs/<timestamp>
tensorboard --logdir logs
```

This displays:
- Real-time training metrics
- Model architecture (if using neural networks)
- Feature distributions
- Embedding visualizations

</details></ul>

<ul><details open><summary><a href="#3-3">3.3 MLFlow</a></summary><a id="3-3"></a>

For experiment tracking across multiple runs:

```bash
cd experiments_outputs
mlflow ui
```

MLFlow tracks:
- Model parameters and hyperparameters
- Performance metrics
- Model artifacts
- Comparison between experiments

</details></ul>

</details>

## Quick Start Examples

### 1. Train Random Forest Model

```bash
python stm32ai_main.py --config config_file_examples/training_config.yaml
```

### 2. Evaluate Model Performance

```bash
python stm32ai_main.py --config config_file_examples/evaluation_config.yaml
```

### 3. Benchmark on STM32

```bash
python stm32ai_main.py --config config_file_examples/benchmarking_config.yaml
```

### 4. Deploy to Hardware

```bash
python stm32ai_main.py --config config_file_examples/deployment_config.yaml
```

## Advanced Features

### Custom Feature Extraction

Add custom features in `preprocessing/feature_extraction.py`:

```python
def custom_feature(signal):
    # Your feature calculation
    return feature_value
```

### Model Optimization

For embedded deployment, use:
- Quantization to 8-bit integers
- Feature selection to reduce input dimensions
- Tree pruning to minimize memory usage

### Real-time Testing

Test the model with live EMG data:

```bash
python utils/realtime_test.py --port COM3 --model saved_models/rf_model.pkl
```

## Troubleshooting

Common issues and solutions:

1. **Poor Classification Accuracy**
   - Check electrode placement and skin preparation
   - Increase training data diversity
   - Tune Random Forest hyperparameters

2. **Memory Constraints**
   - Reduce number of trees or max depth
   - Use feature selection
   - Enable quantization

3. **Real-time Performance**
   - Optimize feature extraction
   - Use fixed-point arithmetic
   - Reduce window overlap

## References

1. Phinyomark et al., "Feature reduction and selection for EMG signal classification"
2. STM32Cube.AI Documentation
3. Random Forest optimization for embedded systems
