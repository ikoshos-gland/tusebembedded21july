/**
 * @file random_forest.h
 * @brief Random Forest classifier for embedded systems
 */

#ifndef RANDOM_FOREST_H
#define RANDOM_FOREST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
// Fixed-point type for memory efficiency (Q8.8 format)
typedef int16_t fixed_point_t;

// Compact tree node structure (8 bytes)
typedef struct __attribute__((packed)) {
    uint8_t feature_idx;      // Feature index (0-29)
    uint8_t node_type;        // Bit 7: is_leaf, Bits 6-0: class_label or reserved
    fixed_point_t threshold;  // Q8.8 fixed-point threshold
    uint8_t left_child;       // Index of left child node
    uint8_t right_child;      // Index of right child node
    uint8_t padding[2];       // Alignment padding
} RF_Node_t;

// Tree structure
typedef struct {
    RF_Node_t nodes[63];      // Maximum 63 nodes per tree
    uint8_t n_nodes;          // Actual number of nodes
    uint8_t root_idx;         // Root node index
} RF_Tree_t;

// Complete Random Forest model
typedef struct {
    RF_Tree_t trees[15];      // Maximum 15 trees
    uint8_t n_trees;          // Actual number of trees
    uint8_t n_features;       // Number of input features
    uint8_t n_classes;        // Number of output classes
    
    // Feature normalization parameters (Q8.8 format)
    fixed_point_t feature_scale[30];
    fixed_point_t feature_offset[30];
} RF_Model_t;

// Voting buffer for temporal smoothing
typedef struct {
    uint8_t predictions[3];   // Last 3 predictions
    uint8_t confidences[3];   // Confidence for each prediction
    uint8_t write_idx;        // Current write position
    uint8_t count;            // Number of valid predictions
} VotingBuffer_t;

/* Exported constants --------------------------------------------------------*/
// Fixed-point configuration
#define FIXED_POINT_FRACTIONAL_BITS  8
#define FIXED_POINT_SCALE           (1 << FIXED_POINT_FRACTIONAL_BITS)

// Model constraints
#define RF_MAX_TREES                15
#define RF_MAX_NODES_PER_TREE       63
#define RF_MAX_FEATURES             30
#define RF_MAX_CLASSES              29  // For Turkish Sign Language

// Node type flags
#define RF_NODE_IS_LEAF             0x80
#define RF_NODE_CLASS_MASK          0x7F

/* Exported macro ------------------------------------------------------------*/
// Fixed-point conversion macros
#define FLOAT_TO_FIXED(x)   ((fixed_point_t)((x) * FIXED_POINT_SCALE + 0.5f))
#define FIXED_TO_FLOAT(x)   ((float)(x) / FIXED_POINT_SCALE)

/* Exported functions prototypes ---------------------------------------------*/
// Model management
HAL_StatusTypeDef RF_LoadModel(void);
HAL_StatusTypeDef RF_GetModelInfo(uint8_t *n_trees, uint8_t *n_features, uint8_t *n_classes);

// Inference functions
uint8_t RF_Predict(const float *features, uint8_t *confidence);
uint8_t RF_PredictFixed(const fixed_point_t *features, uint8_t *confidence);
uint8_t RF_TreePredict(const RF_Tree_t *tree, const fixed_point_t *features);

// Feature normalization
void RF_NormalizeFeatures(const float *raw_features, fixed_point_t *normalized_features, uint8_t n_features);

// Voting functions
void Voting_Init(VotingBuffer_t *buffer);
void Voting_AddPrediction(VotingBuffer_t *buffer, uint8_t prediction, uint8_t confidence);
uint8_t Voting_GetMajority(const VotingBuffer_t *buffer, uint8_t *final_confidence);
void Voting_Reset(VotingBuffer_t *buffer);

// Fixed-point arithmetic
fixed_point_t fixed_mul(fixed_point_t a, fixed_point_t b);
fixed_point_t fixed_div(fixed_point_t a, fixed_point_t b);
fixed_point_t fixed_add(fixed_point_t a, fixed_point_t b);
fixed_point_t fixed_sub(fixed_point_t a, fixed_point_t b);

// Debugging and statistics
void RF_PrintModelStats(void);
void RF_GetMemoryUsage(uint32_t *flash_bytes, uint32_t *ram_bytes);
float RF_GetInferenceTime(void);

// Model validation
bool RF_ValidateModel(const RF_Model_t *model);
bool RF_CheckMemoryConstraints(const RF_Model_t *model);

#ifdef __cplusplus
}
#endif

#endif /* RANDOM_FOREST_H */