import torch
import torch.nn as nn
from .odin_quantizer import ODIN_Quantizer

class ODIN_Linear(nn.Linear):
    def __init__(self, in_features: int, out_features: int, bits: int = 3):
        super().__init__(in_features, out_features, bias = False)
        self.min_val = 0.0
        self.max_val = 2.0 ** bits - 1
        with torch.no_grad():
            mid = (self.max_val + self.min_val) / 2
            scale = (self.max_val - self.min_val) / 6
            nn.init.trunc_normal_(self.weight, mean=mid, std=scale, a=self.min_val, b=self.max_val)

    
    def forward(self, input: torch.Tensor) -> torch.Tensor:
        return nn.functional.linear(input, self.weight_q)
    
    @property
    def weight_q(self):
        return ODIN_Quantizer.apply(self.weight, self.min_val, self.max_val)