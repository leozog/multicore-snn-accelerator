import torch

class SoftThreshold:
    def __init__(self, threshold=0.0):
        self.threshold = threshold

    def __call__(self, x):
        return torch.sign(x) * torch.relu(torch.abs(x) - self.threshold)