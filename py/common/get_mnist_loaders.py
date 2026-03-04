from time import sleep

import torch
from torchvision import datasets, transforms
from torch.utils.data import DataLoader, random_split
from common.dataset_transform_cache import DatasetTransformCache

def get_mnist_loaders(batch_size, transform, fraction: float=1.0):
    print("Starting data loaders")
    train_dataset = datasets.MNIST(root='./data', train=True, download=True)
    if (fraction != 1.0):
        train_dataset, _ = random_split(train_dataset, [fraction, 1-fraction])
        print("train_dataset size:", len(train_dataset))
    train_dataset = DatasetTransformCache(train_dataset, transform)
    train_loader = torch.utils.data.DataLoader(dataset=train_dataset , batch_size=batch_size, shuffle=True, pin_memory=True, num_workers=7, persistent_workers=True, )
    val_dataset = datasets.MNIST(root='./data', train=False, download=True)
    if (fraction != 1.0):
        val_dataset, _ = random_split(val_dataset, [fraction, 1-fraction])
        print("val_dataset size:", len(val_dataset))
    val_dataset = DatasetTransformCache(val_dataset, transform)
    val_loader = torch.utils.data.DataLoader(dataset=val_dataset, batch_size=batch_size, shuffle=False, pin_memory=True, num_workers=7, persistent_workers=True)
    return train_loader, val_loader