
"""
Random Forest model for sEMG-based hand gesture recognition.
Optimized for embedded deployment on STM32 microcontrollers.
"""

import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import cross_val_score, StratifiedKFold
from typing import Dict, Tuple, Optional
import pickle
import struct


class RandomForestEMG:
    """
    Random Forest classifier for EMG gesture recognition.
    Designed with memory constraints for embedded systems.
    """
    
    def __init__(self, 
                 n_estimators: int = 15,
                 max_depth: int = 6,
                 min_samples_split: int = 5,
                 min_samples_leaf: int = 3,
                 max_features: str = 'sqrt',
                 random_state: int = 42):
        """
        Initialize Random Forest with embedded-friendly parameters.
        
        Args:
            n_estimators: Number of trees (10-20 recommended)
            max_depth: Maximum tree depth (4-6 recommended)
            min_samples_split: Minimum samples to split node
            min_samples_leaf: Minimum samples in leaf
            max_features: Number of features to consider
            random_state: Random seed for reproducibility
        """
        self.model = RandomForestClassifier(
            n_estimators=n_estimators,
            max_depth=max_depth,
            min_samples_split=min_samples_split,
            min_samples_leaf=min_samples_leaf,
            max_features=max_features,
            random_state=random_state,
            n_jobs=-1  # Use all CPU cores for training
        )
        
        self.n_features = None
        self.n_classes = None
        self.feature_names = None
        self.class_names = None
        
    def train(self, X_train: np.ndarray, y_train: np.ndarray, 
              feature_names: Optional[list] = None,
              class_names: Optional[list] = None) -> Dict:
        """
        Train the Random Forest model.
        
        Args:
            X_train: Training features (n_samples, n_features)
            y_train: Training labels (n_samples,)
            feature_names: Optional feature names
            class_names: Optional class names
            
        Returns:
            Dictionary with training metrics
        """
        self.n_features = X_train.shape[1]
        self.n_classes = len(np.unique(y_train))
        self.feature_names = feature_names
        self.class_names = class_names
        
        # Train model
        self.model.fit(X_train, y_train)
        
        # Cross-validation
        cv = StratifiedKFold(n_splits=5, shuffle=True, random_state=42)
        cv_scores = cross_val_score(self.model, X_train, y_train, cv=cv, scoring='accuracy')
        
        # Get feature importance
        feature_importance = self.model.feature_importances_
        
        # Calculate memory usage
        memory_stats = self._calculate_memory_usage()
        
        return {
            'cv_accuracy_mean': cv_scores.mean(),
            'cv_accuracy_std': cv_scores.std(),
            'feature_importance': feature_importance,
            'memory_stats': memory_stats
        }
    
    def predict(self, X: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """
        Predict gesture classes with confidence scores.
        
        Args:
            X: Input features (n_samples, n_features)
            
        Returns:
            predictions: Predicted classes
            confidences: Confidence scores (0-100%)
        """
        predictions = self.model.predict(X)
        
        # Get prediction probabilities for confidence
        probabilities = self.model.predict_proba(X)
        confidences = (np.max(probabilities, axis=1) * 100).astype(int)
        
        return predictions, confidences
    
    def _calculate_memory_usage(self) -> Dict:
        """
        Estimate memory usage for embedded deployment.
        
        Returns:
            Dictionary with memory statistics
        """
        total_nodes = 0
        max_depth_actual = 0
        
        for tree in self.model.estimators_:
            total_nodes += tree.tree_.node_count
            max_depth_actual = max(max_depth_actual, tree.tree_.max_depth)
        
        # Estimate memory (8 bytes per node in embedded implementation)
        flash_usage = total_nodes * 8  # bytes
        
        # RAM usage for inference (feature buffer + tree traversal)
        ram_usage = (
            self.n_features * 4 +  # Feature buffer (float32)
            self.n_classes * 1 +   # Vote buffer
            100  # Misc overhead
        )
        
        return {
            'total_nodes': total_nodes,
            'avg_nodes_per_tree': total_nodes / len(self.model.estimators_),
            'max_depth_actual': max_depth_actual,
            'flash_usage_bytes': flash_usage,
            'ram_usage_bytes': ram_usage,
            'flash_usage_kb': flash_usage / 1024,
            'ram_usage_kb': ram_usage / 1024
        }
    
    def export_to_c_header(self, filepath: str, model_name: str = "rf_model"):
        """
        Export model to C header file for embedded deployment.
        
        Args:
            filepath: Output file path
            model_name: Name for the model variable
        """
        with open(filepath, 'w') as f:
            # Write header guard
            f.write(f"#ifndef {model_name.upper()}_H\n")
            f.write(f"#define {model_name.upper()}_H\n\n")
            
            # Write includes
            f.write('#include <stdint.h>\n')
            f.write('#include "random_forest.h"\n\n')
            
            # Write model parameters
            f.write(f"// Model: {model_name}\n")
            f.write(f"// Trees: {len(self.model.estimators_)}\n")
            f.write(f"// Features: {self.n_features}\n")
            f.write(f"// Classes: {self.n_classes}\n\n")
            
            # Write feature normalization parameters (placeholder)
            f.write("// Feature normalization parameters (Q8.8 format)\n")
            f.write(f"const fixed_point_t {model_name}_feature_scale[{self.n_features}] = {{\n")
            for i in range(self.n_features):
                f.write(f"    256,  // Feature {i}\n")
            f.write("};\n\n")
            
            f.write(f"const fixed_point_t {model_name}_feature_offset[{self.n_features}] = {{\n")
            for i in range(self.n_features):
                f.write(f"    0,  // Feature {i}\n")
            f.write("};\n\n")
            
            # Write tree data
            f.write(f"// Random Forest model data\n")
            f.write(f"const RF_Model_t {model_name} = {{\n")
            f.write(f"    .n_trees = {len(self.model.estimators_)},\n")
            f.write(f"    .n_features = {self.n_features},\n")
            f.write(f"    .n_classes = {self.n_classes},\n")
            f.write("    .trees = {\n")
            
            # Export each tree
            for tree_idx, estimator in enumerate(self.model.estimators_):
                tree = estimator.tree_
                f.write(f"        {{ // Tree {tree_idx}\n")
                f.write("            .nodes = {\n")
                
                # Export nodes
                for node_idx in range(tree.node_count):
                    if tree.feature[node_idx] >= 0:  # Internal node
                        feature = tree.feature[node_idx]
                        threshold = int(tree.threshold[node_idx] * 256)  # Convert to Q8.8
                        left = tree.children_left[node_idx]
                        right = tree.children_right[node_idx]
                        
                        f.write(f"                {{{feature}, 0x00, {threshold}, "
                               f"{left}, {right}, {{0, 0}}}},\n")
                    else:  # Leaf node
                        class_label = np.argmax(tree.value[node_idx])
                        f.write(f"                {{0, 0x80 | {class_label}, 0, "
                               f"0, 0, {{0, 0}}}},\n")
                
                f.write("            },\n")
                f.write(f"            .n_nodes = {tree.node_count},\n")
                f.write("            .root_idx = 0\n")
                f.write("        },\n")
            
            f.write("    }\n")
            f.write("};\n\n")
            
            # Write footer
            f.write(f"#endif // {model_name.upper()}_H\n")
    
    def save_model(self, filepath: str):
        """Save model to pickle file."""
        with open(filepath, 'wb') as f:
            pickle.dump(self, f)
    
    @staticmethod
    def load_model(filepath: str):
        """Load model from pickle file."""
        with open(filepath, 'rb') as f:
            return pickle.load(f)


def get_random_forest_model(num_classes: int = 3, 
                           n_features: int = 30,
                           **kwargs) -> RandomForestEMG:
    """
    Factory function to create Random Forest model.
    
    Args:
        num_classes: Number of gesture classes
        n_features: Number of input features
        **kwargs: Additional parameters for RandomForestEMG
        
    Returns:
        RandomForestEMG instance
    """
    # Set default parameters optimized for embedded deployment
    default_params = {
        'n_estimators': 15 if num_classes <= 8 else 20,
        'max_depth': 5 if num_classes <= 8 else 6,
        'min_samples_split': 5,
        'min_samples_leaf': 3,
        'max_features': 'sqrt',
        'random_state': 42
    }
    
    # Update with user parameters
    default_params.update(kwargs)
    
    return RandomForestEMG(**default_params)


# Feature extraction functions for EMG signals
def extract_time_domain_features(signal: np.ndarray) -> Dict[str, float]:
    """
    Extract time-domain features from EMG signal.
    
    Args:
        signal: EMG signal array
        
    Returns:
        Dictionary of features
    """
    features = {}
    
    # RMS (Root Mean Square)
    features['rms'] = np.sqrt(np.mean(signal**2))
    
    # MAV (Mean Absolute Value)
    features['mav'] = np.mean(np.abs(signal))
    
    # VAR (Variance)
    features['var'] = np.var(signal)
    
    # ZC (Zero Crossings)
    zero_crossings = np.where(np.diff(np.sign(signal)))[0]
    features['zc'] = len(zero_crossings)
    
    # SSC (Slope Sign Changes)
    diff_signal = np.diff(signal)
    ssc = np.where(np.diff(np.sign(diff_signal)))[0]
    features['ssc'] = len(ssc)
    
    # WL (Waveform Length)
    features['wl'] = np.sum(np.abs(np.diff(signal)))
    
    return features


def extract_frequency_domain_features(signal: np.ndarray, 
                                    fs: float = 1000.0) -> Dict[str, float]:
    """
    Extract frequency-domain features from EMG signal using FFT.
    
    Args:
        signal: EMG signal array
        fs: Sampling frequency
        
    Returns:
        Dictionary of features
    """
    features = {}
    
    # Compute FFT
    fft = np.fft.rfft(signal)
    freqs = np.fft.rfftfreq(len(signal), 1/fs)
    magnitude = np.abs(fft)
    psd = magnitude**2
    
    # Mean frequency
    features['mean_freq'] = np.sum(freqs * psd) / np.sum(psd)
    
    # Median frequency
    cumsum = np.cumsum(psd)
    median_idx = np.where(cumsum >= cumsum[-1] / 2)[0][0]
    features['median_freq'] = freqs[median_idx]
    
    # Band power (0-50Hz, 50-150Hz, 150-250Hz, 250-500Hz)
    bands = [(0, 50), (50, 150), (150, 250), (250, 500)]
    for i, (low, high) in enumerate(bands):
        band_idx = np.logical_and(freqs >= low, freqs <= high)
        features[f'band_power_{i}'] = np.sum(psd[band_idx])
    
    return features