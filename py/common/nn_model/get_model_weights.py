import torch
import torch.nn as nn

def get_model_weights(model):
    print("Extracting model weights...")
    weights = []
    for layerIdx, layer in enumerate(model.network.children()):
        try:
            print(layer)
            # weight = layer.weight if layer.weight_int == None else layer.weitht_int
            weight = layer.weight_int
            for toNeuronIdx, toNeuron in enumerate(weight):
                for fromNeuronIdx, weight in enumerate(toNeuron):
                    weights.append({
                        "from": (layerIdx - 1, fromNeuronIdx),
                        "to": (layerIdx + 1, toNeuronIdx),
                        "weight": weight
                    })
        except AttributeError:
            print(f"Skipping layer {layerIdx}")
            continue
    return weights