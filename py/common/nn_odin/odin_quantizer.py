import torch

class ODIN_Quantizer(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input: torch.Tensor, min_val, max_val) -> torch.Tensor:
        min_val = torch.ceil(torch.as_tensor(min_val, device=input.device))
        max_val = torch.floor(torch.as_tensor(max_val, device=input.device))
        ctx.save_for_backward(input, min_val, max_val)
        return torch.clamp(torch.round(input), min_val, max_val)

    @staticmethod
    def backward(ctx, grad_output: torch.Tensor) -> torch.Tensor:
        (input, min_val, max_val) = ctx.saved_tensors
        grad = grad_output.clone()
        grad[input < min_val] = 0
        grad[input > max_val] = 0
        return grad, None, None # The n of outputs needs to be the same as n of inputs to forward().