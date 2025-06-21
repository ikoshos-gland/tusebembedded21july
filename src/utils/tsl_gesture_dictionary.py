
"""
Turkish Sign Language (TSL) gesture dictionary for sEMG hand prosthesis.
Maps gesture names to class IDs and servo positions.
"""

# Initial 3-class gesture dictionary
basic_gesture_dict = {
    "open_hand": 0,      # All fingers extended
    "closed_fist": 1,    # All fingers closed
    "peace_sign": 2,     # Index and middle extended
}

# Full Turkish Sign Language alphabet (29 letters)
tsl_alphabet_dict = {
    # Turkish alphabet letters
    "A": 0,
    "B": 1,
    "C": 2,
    "Ç": 3,
    "D": 4,
    "E": 5,
    "F": 6,
    "G": 7,
    "Ğ": 8,
    "H": 9,
    "I": 10,
    "İ": 11,
    "J": 12,
    "K": 13,
    "L": 14,
    "M": 15,
    "N": 16,
    "O": 17,
    "Ö": 18,
    "P": 19,
    "R": 20,
    "S": 21,
    "Ş": 22,
    "T": 23,
    "U": 24,
    "Ü": 25,
    "V": 26,
    "Y": 27,
    "Z": 28,
}

# Extended gesture set (future expansion)
extended_gesture_dict = {
    # Numbers
    "one": 29,
    "two": 30,
    "three": 31,
    "four": 32,
    "five": 33,
    "six": 34,
    "seven": 35,
    "eight": 36,
    "nine": 37,
    "zero": 38,
    
    # Common words/phrases
    "hello": 39,
    "goodbye": 40,
    "thank_you": 41,
    "please": 42,
    "yes": 43,
    "no": 44,
    "help": 45,
    "stop": 46,
    "go": 47,
    "wait": 48,
}

# Servo position mappings for each gesture
# Format: [thumb, index, middle, ring, pinky, wrist]
# Values: 0-180 degrees

basic_servo_positions = {
    "open_hand": [180, 180, 180, 180, 180, 90],      # All extended
    "closed_fist": [0, 0, 0, 0, 0, 90],              # All closed
    "peace_sign": [0, 180, 180, 0, 0, 90],           # Victory sign
}

# TSL alphabet servo positions (simplified examples)
tsl_servo_positions = {
    "A": [0, 0, 0, 0, 0, 90],          # Closed fist with thumb on side
    "B": [180, 180, 180, 180, 180, 90], # Open palm
    "C": [90, 90, 90, 90, 90, 90],     # Curved hand
    "D": [0, 180, 0, 0, 0, 90],        # Index pointing up
    "E": [90, 0, 0, 0, 0, 90],         # Fingers bent at 90 degrees
    # ... Add all 29 letters with proper servo positions
}

# Gesture transition timing (milliseconds)
gesture_transition_times = {
    "default": 200,      # Default transition time
    "fast": 100,         # Quick transitions
    "slow": 500,         # Slow, smooth transitions
    "emergency": 50,     # Emergency stop
}

# Gesture confidence thresholds
confidence_thresholds = {
    "high": 80,          # High confidence required
    "medium": 60,        # Medium confidence
    "low": 40,           # Low confidence (training mode)
}

def get_gesture_id(gesture_name: str, dictionary: str = "basic") -> int:
    """
    Get gesture ID from name.
    
    Args:
        gesture_name: Name of the gesture
        dictionary: Which dictionary to use ("basic", "tsl", "extended")
        
    Returns:
        Gesture ID or -1 if not found
    """
    if dictionary == "basic":
        return basic_gesture_dict.get(gesture_name, -1)
    elif dictionary == "tsl":
        return tsl_alphabet_dict.get(gesture_name, -1)
    elif dictionary == "extended":
        return extended_gesture_dict.get(gesture_name, -1)
    else:
        return -1

def get_servo_positions(gesture_name: str, dictionary: str = "basic") -> list:
    """
    Get servo positions for a gesture.
    
    Args:
        gesture_name: Name of the gesture
        dictionary: Which dictionary to use
        
    Returns:
        List of 6 servo positions [thumb, index, middle, ring, pinky, wrist]
    """
    if dictionary == "basic":
        return basic_servo_positions.get(gesture_name, [90, 90, 90, 90, 90, 90])
    elif dictionary == "tsl":
        return tsl_servo_positions.get(gesture_name, [90, 90, 90, 90, 90, 90])
    else:
        return [90, 90, 90, 90, 90, 90]  # Default neutral position

def get_all_gestures(dictionary: str = "basic") -> dict:
    """
    Get all gestures from specified dictionary.
    
    Args:
        dictionary: Which dictionary to use
        
    Returns:
        Dictionary of gesture names and IDs
    """
    if dictionary == "basic":
        return basic_gesture_dict
    elif dictionary == "tsl":
        return tsl_alphabet_dict
    elif dictionary == "extended":
        return extended_gesture_dict
    else:
        return {}

# Export for C header generation
def export_gesture_map_to_c(filepath: str, dictionary: str = "basic"):
    """
    Export gesture mappings to C header file.
    
    Args:
        filepath: Output file path
        dictionary: Which dictionary to export
    """
    gestures = get_all_gestures(dictionary)
    
    with open(filepath, 'w') as f:
        f.write("// Auto-generated gesture mappings\n")
        f.write("#ifndef GESTURE_MAP_H\n")
        f.write("#define GESTURE_MAP_H\n\n")
        
        # Write gesture IDs
        f.write("// Gesture IDs\n")
        for name, id in gestures.items():
            f.write(f"#define GESTURE_{name.upper()} {id}\n")
        
        f.write(f"\n#define NUM_GESTURES {len(gestures)}\n\n")
        
        # Write servo positions
        f.write("// Servo positions for each gesture\n")
        f.write("const uint8_t gesture_servo_positions[NUM_GESTURES][6] = {\n")
        
        positions = basic_servo_positions if dictionary == "basic" else tsl_servo_positions
        for name, id in sorted(gestures.items(), key=lambda x: x[1]):
            pos = positions.get(name, [90, 90, 90, 90, 90, 90])
            f.write(f"    {{{pos[0]}, {pos[1]}, {pos[2]}, {pos[3]}, {pos[4]}, {pos[5]}}}, // {name}\n")
        
        f.write("};\n\n")
        
        # Write gesture names
        f.write("// Gesture names\n")
        f.write("const char* gesture_names[NUM_GESTURES] = {\n")
        for name, id in sorted(gestures.items(), key=lambda x: x[1]):
            f.write(f'    "{name}",\n')
        f.write("};\n\n")
        
        f.write("#endif // GESTURE_MAP_H\n")