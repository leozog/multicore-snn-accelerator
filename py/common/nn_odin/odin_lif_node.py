
import torch.nn as nn
from typing import Callable
import torch
from spikingjelly.activation_based import neuron, surrogate
from .odin_quantizer import ODIN_Quantizer

class ODIN_LIFNode(neuron.BaseNode):
    def __init__(self, n_neurons: int = 1, init_v_thr: float = 1., init_leak_str: float = 0.0, max_spikes: int = 8,
                 surrogate_function: Callable = surrogate.ATan(alpha=0.25), step_mode='s',
                 backend='torch', store_v_seq: bool = False):
        super().__init__(
            v_threshold=1.0, 
            v_reset=None, # Use soft reset
            surrogate_function=surrogate_function, 
            detach_reset=True, 
            step_mode=step_mode, 
            backend=backend, 
            store_v_seq=store_v_seq
            )
        self.V_THR_MIN = 1
        self.V_THR_MAX = 2 ** 8 - 1
        self.LEAK_STR_MIN = 0
        self.LEAK_STR_MAX = 2 ** 7 - 1
        self.max_spikes = max_spikes
        self.n_neurons = n_neurons
        self.v_thr = nn.Parameter(torch.clamp(torch.randn(n_neurons) * 1. + init_v_thr, min=self.V_THR_MIN, max=self.V_THR_MAX))
        self.leak_str = nn.Parameter(torch.clamp(torch.randn(n_neurons) * 1. + init_leak_str, min=self.LEAK_STR_MIN, max=self.LEAK_STR_MAX))

    def neuronal_charge(self, x: torch.Tensor):
        self.v = torch.relu(self.v - self.leak_str_q) + x

    def neuronal_fire(self):
        # v_thr_q = self.v_thr_q
        # spikes = torch.zeros_like(self.v)
        # for k in range(1, self.max_spikes + 1):
        #     spike = self.surrogate_function(self.v - k * v_thr_q)
        #     spikes += spike
        # return spikes
        k = torch.arange(
            1, self.max_spikes + 1, 
            device=self.v.device, dtype=self.v.dtype
        ).view(-1, 1, 1)
        spikes = self.surrogate_function(
            torch.addcmul(self.v, k, self.v_thr_q, value=-1)
        ).sum(dim=0)
        return spikes

    @property
    def v_thr_q(self):
        return ODIN_Quantizer.apply(self.v_thr, self.V_THR_MIN, self.V_THR_MAX)
    
    @property
    def leak_str_q(self):
        t = ODIN_Quantizer.apply(self.leak_str, torch.zeros_like(self.v_thr_q), self.v_thr_q)
        return ODIN_Quantizer.apply(t, self.LEAK_STR_MIN, self.LEAK_STR_MAX)