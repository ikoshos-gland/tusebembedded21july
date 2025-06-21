
from typing import Dict
from tensorflow import keras
from tensorflow.keras import layers



def get_data_augmentation(data_augmentation: Dict = None):
    """ Define Sequential data-augmentation layers model. """

    augmentation_layers = []
    if data_augmentation.config['random_flip']:
        augmentation_layers.append(layers.RandomFlip(data_augmentation.config['random_flip']['mode']))

    data_augmentation = keras.Sequential(augmentation_layers)
    data_augmentation._name = "Data_augmentation"

    return data_augmentation

