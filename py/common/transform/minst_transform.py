import torch
import torch.nn as nn
from torchvision.transforms import v2 as transforms
from .deskew.deskew import Deskew
from .soft_threshold import SoftThreshold

class ResizeDeskewTransform:
    def __init__(self, resize: tuple[int, int]):
        self.transform = transforms.Compose([
            transforms.Resize(resize, antialias=True, interpolation=transforms.InterpolationMode.BICUBIC),
            Deskew(),
            transforms.ToImage(),
            transforms.ToDtype(torch.float32, scale=True),
        ])

    def __call__(self, img):
        return self.transform(img)

# class SoftThresholdNormalizeTransform:
#     def __init__(self, threshold: float):
#         self.transform = transforms.Compose([
#             SoftThreshold(threshold=threshold),
#             transforms.Normalize([0.0], [1.0 - threshold])
#         ])

#     def __call__(self, img):
#         return self.transform(img)

class ResizeDeskewThresholdTransform(ResizeDeskewTransform):
    def __init__(self, resize: tuple[int, int], threshold: float):
        super().__init__(resize)
        self.transform = transforms.Compose([
            self.transform,
            SoftThreshold(threshold=threshold)
        ])