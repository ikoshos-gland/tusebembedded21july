# Random Forest for Embedded Systems - Implementation Guide

## Overview

This guide details the implementation of a memory-efficient Random Forest classifier for the STM32H7S3L8 MCU, optimized for sEMG gesture recognition with strict memory constraints (<12KB RAM, <32KB Flash).

## 1. Model Architecture

### 1.1 Memory-Optimized Tree Structure

```c
// Fixed-point representation for thresholds (Q8.8 format)
// Range: -128.0 to 127.996 with 0.0039 precision
typedef int16_t fixed_point_t;

// Compact node structure (8 bytes per node)
typedef struct __attribute__((packed)) {
    uint8_t  feature_idx;      // Feature index (0-29)
    uint8_t  node_type;        // Bit 7: is_leaf, Bits 6-0: class_label or reserved
    fixed_point_t threshold;   // Q8.8 fixed-point threshold
    uint8_t  left_child;       // Index of left child
    uint8_t  right_child;      // Index of right child
    uint8_t  padding[2];       // Alignment to 8 bytes
} RF_Node_t;

// Tree structure with compile-time bounds
#define MAX_NODES_PER_TREE 63  // Allows 6-level deep trees
#define MAX_TREES 15           // Balance between accuracy and memory

typedef struct {
    RF_Node_t nodes[MAX_NODES_PER_TREE];
    uint8_t   n_nodes;
    uint8_t   root_idx;
} RF_Tree_t;

// Complete Random Forest model
typedef struct {
    RF_Tree_t trees[MAX_TREES];
    uint8_t   n_trees;
    uint8_t   n_features;
    uint8_t   n_classes;
    
    // Feature normalization parameters (Q8.8 format)
    fixed_point_t feature_scale[30];
    fixed_point_t feature_offset[30];
    
    // Class names for debugging (optional, can be removed for production)
    #ifdef DEBUG_MODE
    char class_names[29][16];  // For Turkish Sign Language letters
    #endif
} RF_Model_t;
```

### 1.2 Memory Layout

```
Total Model Size Calculation:
- Per Node: 8 bytes
- Per Tree: 63 nodes × 8 bytes = 504 bytes
- 15 Trees: 15 × 504 = 7,560 bytes
- Feature params: 30 × 2 × 2 bytes = 120 bytes
- Overhead: ~100 bytes
- Total: ~7.8KB in Flash (well under 32KB limit)

Runtime Memory:
- Feature vector: 30 × 4 bytes = 120 bytes
- Tree traversal stack: 15 × 8 bytes = 120 bytes
- Voting buffer: 29 × 1 byte = 29 bytes
- Working memory: ~500 bytes
- Total: <1KB RAM for inference
```

## 2. Fixed-Point Arithmetic Implementation

### 2.1 Q8.8 Format Operations

```c
// Q8.8 fixed-point configuration
#define FIXED_POINT_FRACTIONAL_BITS 8
#define FIXED_POINT_SCALE (1 << FIXED_POINT_FRACTIONAL_BITS)

// Conversion macros
#define FLOAT_TO_FIXED(x) ((fixed_point_t)((x) * FIXED_POINT_SCALE + 0.5f))
#define FIXED_TO_FLOAT(x) ((float)(x) / FIXED_POINT_SCALE)

// Fixed-point multiplication
static inline fixed_point_t fixed_mul(fixed_point_t a, fixed_point_t b) {
    int32_t temp = (int32_t)a * (int32_t)b;
    return (fixed_point_t)(temp >> FIXED_POINT_FRACTIONAL_BITS);
}

// Fixed-point division (avoided when possible)
static inline fixed_point_t fixed_div(fixed_point_t a, fixed_point_t b) {
    int32_t temp = ((int32_t)a << FIXED_POINT_FRACTIONAL_BITS);
    return (fixed_point_t)(temp / b);
}

// Feature normalization in fixed-point
static inline fixed_point_t normalize_feature(float raw_value, 
                                             fixed_point_t scale, 
                                             fixed_point_t offset) {
    fixed_point_t fixed_raw = FLOAT_TO_FIXED(raw_value);
    fixed_point_t normalized = fixed_mul(fixed_raw - offset, scale);
    return normalized;
}
```

### 2.2 Optimized Tree Traversal

```c
// Stack-based iterative traversal (no recursion to save stack space)
uint8_t RF_TreePredict(const RF_Tree_t* tree, const fixed_point_t* features) {
    uint8_t node_idx = tree->root_idx;
    
    while (1) {
        const RF_Node_t* node = &tree->nodes[node_idx];
        
        // Check if leaf node (bit 7 of node_type)
        if (node->node_type & 0x80) {
            return node->node_type & 0x7F;  // Return class label
        }
        
        // Compare feature with threshold
        if (features[node->feature_idx] <= node->threshold) {
            node_idx = node->left_child;
        } else {
            node_idx = node->right_child;
        }
        
        // Safety check to prevent infinite loops
        if (node_idx >= tree->n_nodes) {
            return 0;  // Default class
        }
    }
}
```

## 3. Inference Pipeline

### 3.1 Complete Inference Function

```c
// Optimized inference with majority voting
uint8_t RF_Predict(const RF_Model_t* model, 
                   const float* raw_features,
                   uint8_t* confidence) {
    // Step 1: Normalize features to fixed-point
    fixed_point_t normalized_features[30];
    for (uint8_t i = 0; i < model->n_features; i++) {
        normalized_features[i] = normalize_feature(
            raw_features[i],
            model->feature_scale[i],
            model->feature_offset[i]
        );
    }
    
    // Step 2: Collect predictions from all trees
    uint8_t votes[29] = {0};  // Vote counter for each class
    
    for (uint8_t t = 0; t < model->n_trees; t++) {
        uint8_t prediction = RF_TreePredict(&model->trees[t], normalized_features);
        if (prediction < model->n_classes) {
            votes[prediction]++;
        }
    }
    
    // Step 3: Find majority class
    uint8_t best_class = 0;
    uint8_t max_votes = 0;
    
    for (uint8_t c = 0; c < model->n_classes; c++) {
        if (votes[c] > max_votes) {
            max_votes = votes[c];
            best_class = c;
        }
    }
    
    // Step 4: Calculate confidence (0-100%)
    if (confidence != NULL) {
        *confidence = (max_votes * 100) / model->n_trees;
    }
    
    return best_class;
}
```

### 3.2 Sliding Window Majority Voting

```c
// Circular buffer for temporal smoothing
typedef struct {
    uint8_t predictions[3];
    uint8_t confidences[3];
    uint8_t write_idx;
    uint8_t count;
} VotingBuffer_t;

// Add new prediction to voting buffer
void Voting_AddPrediction(VotingBuffer_t* buffer, 
                         uint8_t prediction, 
                         uint8_t confidence) {
    buffer->predictions[buffer->write_idx] = prediction;
    buffer->confidences[buffer->write_idx] = confidence;
    buffer->write_idx = (buffer->write_idx + 1) % 3;
    if (buffer->count < 3) buffer->count++;
}

// Get majority vote with confidence weighting
uint8_t Voting_GetMajority(const VotingBuffer_t* buffer, 
                          uint8_t* final_confidence) {
    if (buffer->count == 0) return 0;
    
    // Weighted voting based on confidence
    uint16_t weighted_votes[29] = {0};
    uint16_t total_weight = 0;
    
    for (uint8_t i = 0; i < buffer->count; i++) {
        uint8_t pred = buffer->predictions[i];
        uint8_t conf = buffer->confidences[i];
        weighted_votes[pred] += conf;
        total_weight += conf;
    }
    
    // Find best class
    uint8_t best_class = 0;
    uint16_t max_weight = 0;
    
    for (uint8_t c = 0; c < 29; c++) {
        if (weighted_votes[c] > max_weight) {
            max_weight = weighted_votes[c];
            best_class = c;
        }
    }
    
    if (final_confidence != NULL && total_weight > 0) {
        *final_confidence = (max_weight * 100) / total_weight;
    }
    
    return best_class;
}
```

## 4. Model Training (Python)

### 4.1 Feature Extraction Pipeline

```python
import numpy as np
from scipy import signal
from sklearn.ensemble import RandomForestClassifier
import struct

class EMGFeatureExtractor:
    def __init__(self, sampling_rate=1000, window_size=256):
        self.fs = sampling_rate
        self.window_size = window_size
        self.nyquist = sampling_rate / 2
        
        # Bandpass filter design (20-500 Hz)
        self.b, self.a = signal.butter(4, [20/self.nyquist, 500/self.nyquist], 'band')
        
        # Hamming window for STFT
        self.hamming = np.hamming(64)
        
    def extract_features(self, emg_data, acc_data=None):
        """
        Extract features from 4-channel EMG data
        emg_data: shape (256, 4) - 256 samples, 4 channels
        acc_data: shape (26,) - 26 samples at 104 Hz (optional)
        """
        features = []
        
        for ch in range(4):
            channel_data = emg_data[:, ch]
            
            # Apply bandpass filter
            filtered = signal.filtfilt(self.b, self.a, channel_data)
            
            # Time-domain features
            features.append(self._rms(filtered))
            features.append(self._mav(filtered))
            features.append(self._var(filtered))
            features.append(self._zc(filtered))
            features.append(self._ssc(filtered))
            features.append(self._wl(filtered))
            
            # Frequency-domain features (STFT)
            f, t, Zxx = signal.stft(filtered, self.fs, window='hamming', 
                                   nperseg=64, noverlap=32)
            
            # Average across time windows
            power_spectrum = np.mean(np.abs(Zxx)**2, axis=1)
            
            # Spectral features
            features.append(self._mean_frequency(f, power_spectrum))
            features.append(self._median_frequency(f, power_spectrum))
            
            # Power in frequency bands
            bands = [(0, 50), (50, 150), (150, 250), (250, 500)]
            for low, high in bands:
                features.append(self._band_power(f, power_spectrum, low, high))
        
        # Accelerometer features (if available)
        if acc_data is not None:
            features.append(np.mean(acc_data))  # Mean acceleration
            features.append(np.std(acc_data))   # Acceleration variance
        
        return np.array(features)
    
    # Time-domain feature functions
    def _rms(self, x):
        return np.sqrt(np.mean(x**2))
    
    def _mav(self, x):
        return np.mean(np.abs(x))
    
    def _var(self, x):
        return np.var(x)
    
    def _zc(self, x, threshold=0.01):
        return np.sum(np.diff(np.sign(x)) != 0)
    
    def _ssc(self, x):
        return np.sum(np.diff(np.sign(np.diff(x))) != 0)
    
    def _wl(self, x):
        return np.sum(np.abs(np.diff(x)))
    
    # Frequency-domain feature functions
    def _mean_frequency(self, f, psd):
        return np.sum(f * psd) / np.sum(psd)
    
    def _median_frequency(self, f, psd):
        cumsum = np.cumsum(psd)
        return f[np.where(cumsum >= cumsum[-1] / 2)[0][0]]
    
    def _band_power(self, f, psd, low, high):
        idx = np.logical_and(f >= low, f <= high)
        return np.sum(psd[idx])
```

### 4.2 Model Training and Optimization

```python
class EmbeddedRandomForest:
    def __init__(self, max_trees=15, max_depth=6, max_nodes=63):
        self.max_trees = max_trees
        self.max_depth = max_depth
        self.max_nodes = max_nodes
        
    def train(self, X_train, y_train, X_val, y_val):
        """Train RF model with embedded constraints"""
        
        # Grid search for optimal parameters
        best_score = 0
        best_model = None
        
        for n_trees in [10, 12, 15]:
            for max_depth in [4, 5, 6]:
                rf = RandomForestClassifier(
                    n_estimators=n_trees,
                    max_depth=max_depth,
                    max_leaf_nodes=self.max_nodes // 2,  # Ensure we don't exceed node limit
                    min_samples_split=5,  # Prevent overfitting
                    min_samples_leaf=3,
                    random_state=42
                )
                
                rf.fit(X_train, y_train)
                score = rf.score(X_val, y_val)
                
                # Check if model fits memory constraints
                if self._check_memory_constraints(rf) and score > best_score:
                    best_score = score
                    best_model = rf
        
        print(f"Best validation accuracy: {best_score:.2%}")
        return best_model
    
    def _check_memory_constraints(self, model):
        """Verify model fits in embedded memory"""
        total_nodes = sum(tree.tree_.node_count for tree in model.estimators_)
        total_memory = total_nodes * 8  # 8 bytes per node
        
        return total_memory < 30 * 1024  # 30KB limit
    
    def export_to_c(self, model, feature_scaler, output_file='rf_model.h'):
        """Export trained model to C header file"""
        
        with open(output_file, 'w') as f:
            f.write("// Auto-generated Random Forest model\n")
            f.write("#ifndef RF_MODEL_H\n")
            f.write("#define RF_MODEL_H\n\n")
            
            # Write model parameters
            f.write(f"#define RF_N_TREES {len(model.estimators_)}\n")
            f.write(f"#define RF_N_FEATURES {model.n_features_in_}\n")
            f.write(f"#define RF_N_CLASSES {model.n_classes_}\n\n")
            
            # Write feature normalization parameters
            f.write("// Feature normalization (Q8.8 fixed-point)\n")
            f.write("const fixed_point_t rf_feature_scale[30] = {\n")
            for i, scale in enumerate(feature_scaler.scale_):
                fixed_scale = int(scale * 256)  # Convert to Q8.8
                f.write(f"    {fixed_scale},  // Feature {i}\n")
            f.write("};\n\n")
            
            f.write("const fixed_point_t rf_feature_offset[30] = {\n")
            for i, offset in enumerate(feature_scaler.mean_):
                fixed_offset = int(offset * 256)  # Convert to Q8.8
                f.write(f"    {fixed_offset},  // Feature {i}\n")
            f.write("};\n\n")
            
            # Write tree data
            f.write("// Tree structures\n")
            f.write("const RF_Tree_t rf_trees[RF_N_TREES] = {\n")
            
            for tree_idx, tree in enumerate(model.estimators_):
                f.write(f"    {{ // Tree {tree_idx}\n")
                f.write("        .nodes = {\n")
                
                # Export tree nodes
                tree_data = tree.tree_
                for node_idx in range(tree_data.node_count):
                    if tree_data.feature[node_idx] >= 0:  # Internal node
                        feature = tree_data.feature[node_idx]
                        threshold = int(tree_data.threshold[node_idx] * 256)  # Q8.8
                        left = tree_data.children_left[node_idx]
                        right = tree_data.children_right[node_idx]
                        
                        f.write(f"            {{{feature}, 0x00, {threshold}, "
                               f"{left}, {right}, {{0, 0}}}},\n")
                    else:  # Leaf node
                        class_label = np.argmax(tree_data.value[node_idx])
                        f.write(f"            {{0, 0x80 | {class_label}, 0, "
                               f"0, 0, {{0, 0}}}},\n")
                
                f.write("        },\n")
                f.write(f"        .n_nodes = {tree_data.node_count},\n")
                f.write("        .root_idx = 0\n")
                f.write("    },\n")
            
            f.write("};\n\n")
            f.write("#endif // RF_MODEL_H\n")
```

### 4.3 Model Quantization and Compression

```python
def quantize_model(model, X_calibration, bits=8):
    """
    Quantize model to reduce memory footprint
    """
    # Collect all thresholds
    all_thresholds = []
    for tree in model.estimators_:
        tree_data = tree.tree_
        for i in range(tree_data.node_count):
            if tree_data.feature[i] >= 0:
                all_thresholds.append(tree_data.threshold[i])
    
    # Find quantization range
    min_val = np.min(all_thresholds)
    max_val = np.max(all_thresholds)
    
    # Create quantization scale
    scale = (max_val - min_val) / (2**bits - 1)
    
    # Test quantized model accuracy
    original_score = model.score(X_calibration, y_calibration)
    
    # Apply quantization
    for tree in model.estimators_:
        tree_data = tree.tree_
        for i in range(tree_data.node_count):
            if tree_data.feature[i] >= 0:
                # Quantize threshold
                original = tree_data.threshold[i]
                quantized = round((original - min_val) / scale) * scale + min_val
                tree_data.threshold[i] = quantized
    
    quantized_score = model.score(X_calibration, y_calibration)
    print(f"Accuracy drop from quantization: {original_score - quantized_score:.2%}")
    
    return model, scale, min_val
```

## 5. Real-Time Integration

### 5.1 FreeRTOS Task Implementation

```c
// Global model instance (stored in Flash)
extern const RF_Model_t rf_model;

// Inference task
void ML_InferenceTask(void *pvParameters) {
    Feature_Vector_t features;
    VotingBuffer_t voting_buffer = {0};
    uint8_t gesture_class;
    uint8_t confidence;
    
    // Task timing
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(128);  // 128ms period
    
    while (1) {
        // Wait for next period
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
        
        // Get latest feature vector from DSP task
        if (xQueueReceive(feature_queue, &features, 0) == pdTRUE) {
            
            // Run inference
            uint8_t prediction = RF_Predict(&rf_model, 
                                          features.values, 
                                          &confidence);
            
            // Add to voting buffer
            Voting_AddPrediction(&voting_buffer, prediction, confidence);
            
            // Get final gesture with temporal smoothing
            uint8_t final_confidence;
            gesture_class = Voting_GetMajority(&voting_buffer, 
                                             &final_confidence);
            
            // Only update servos if confidence is high enough
            if (final_confidence > 70) {
                Servo_UpdateGesture(gesture_class);
                
                // Log prediction
                LOG_INFO("Gesture: %d, Confidence: %d%%", 
                        gesture_class, final_confidence);
            }
        }
    }
}
```

### 5.2 Performance Profiling

```c
// Timing measurement for optimization
typedef struct {
    uint32_t feature_extraction_us;
    uint32_t inference_us;
    uint32_t total_predictions;
    uint32_t correct_predictions;
} ML_Performance_t;

static ML_Performance_t perf_stats = {0};

// Measure inference time
void ML_ProfileInference(void) {
    uint32_t start_time = DWT->CYCCNT;
    
    // Run inference
    uint8_t result = RF_Predict(&rf_model, test_features, NULL);
    
    uint32_t end_time = DWT->CYCCNT;
    uint32_t cycles = end_time - start_time;
    
    // Convert to microseconds (assuming 280MHz clock)
    uint32_t time_us = cycles / 280;
    
    perf_stats.inference_us = 
        (perf_stats.inference_us * perf_stats.total_predictions + time_us) / 
        (perf_stats.total_predictions + 1);
    
    perf_stats.total_predictions++;
}
```

## 6. Memory Optimization Techniques

### 6.1 Flash Storage with XIP

```c
// Store model in Flash with proper alignment
__attribute__((section(".rf_model"), aligned(8)))
const RF_Model_t rf_model = {
    #include "rf_model_data.inc"  // Generated by Python script
};

// Enable Flash cache and prefetch for faster access
void Flash_OptimizeForXIP(void) {
    // Enable instruction cache
    SCB_EnableICache();
    
    // Enable data cache
    SCB_EnableDCache();
    
    // Configure Flash latency
    FLASH->ACR |= FLASH_ACR_LATENCY_5WS;
    
    // Enable prefetch
    FLASH->ACR |= FLASH_ACR_PRFTEN;
}
```

### 6.2 RAM Usage Optimization

```c
// Use static allocation to avoid heap fragmentation
static fixed_point_t feature_buffer[30] __attribute__((aligned(4)));
static uint8_t vote_buffer[29] __attribute__((aligned(4)));

// Pack structures to minimize padding
typedef struct __attribute__((packed)) {
    uint16_t sample_count;
    uint8_t channel_mask;
    uint8_t status;
} EMG_Status_t;

// Use bit fields for flags
typedef struct {
    uint32_t gesture_detected : 1;
    uint32_t high_confidence : 1;
    uint32_t buffer_overflow : 1;
    uint32_t reserved : 29;
} System_Flags_t;
```

## 7. Testing and Validation

### 7.1 Unit Tests

```c
// Test fixed-point arithmetic
void Test_FixedPoint(void) {
    // Test conversion
    float test_val = 3.14159f;
    fixed_point_t fixed = FLOAT_TO_FIXED(test_val);
    float recovered = FIXED_TO_FLOAT(fixed);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, test_val, recovered);
    
    // Test multiplication
    fixed_point_t a = FLOAT_TO_FIXED(2.5f);
    fixed_point_t b = FLOAT_TO_FIXED(1.5f);
    fixed_point_t result = fixed_mul(a, b);
    float expected = 3.75f;
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expected, FIXED_TO_FLOAT(result));
}

// Test model inference
void Test_ModelInference(void) {
    // Known test vector
    float test_features[30] = {
        // Pre-calculated feature values
        0.5f, 0.3f, 0.7f, /* ... more features ... */
    };
    
    uint8_t confidence;
    uint8_t result = RF_Predict(&rf_model, test_features, &confidence);
    
    // Verify result matches Python model
    TEST_ASSERT_EQUAL_UINT8(expected_class, result);
    TEST_ASSERT_GREATER_THAN_UINT8(70, confidence);
}
```

### 7.2 Integration Tests

```python
# Bit-exact verification between Python and C
def verify_c_implementation(model, test_data, c_executable):
    """
    Compare Python and C implementations
    """
    import subprocess
    import json
    
    results_match = True
    
    for i, (features, expected) in enumerate(test_data):
        # Python prediction
        python_pred = model.predict([features])[0]
        
        # C prediction (via test executable)
        result = subprocess.run(
            [c_executable, '--features', json.dumps(features.tolist())],
            capture_output=True,
            text=True
        )
        c_pred = int(result.stdout.strip())
        
        if python_pred != c_pred:
            print(f"Mismatch at sample {i}: Python={python_pred}, C={c_pred}")
            results_match = False
    
    return results_match
```

## 8. Deployment Checklist

- [ ] Model fits within memory constraints (<12KB RAM, <32KB Flash)
- [ ] Inference time <3ms on target MCU
- [ ] Fixed-point accuracy within 2% of floating-point
- [ ] All 3 initial gesture classes achieve >90% accuracy
- [ ] Temporal voting improves stability
- [ ] No memory leaks or stack overflows
- [ ] Power consumption within budget
- [ ] Real-time deadlines consistently met
- [ ] Error handling for edge cases implemented
- [ ] Model update mechanism in place for future expansion