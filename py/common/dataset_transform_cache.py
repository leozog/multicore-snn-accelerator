import torch
from functools import lru_cache

class DatasetTransformCache(torch.utils.data.Dataset):
    def __init__(self, dataset, transform):
        self.dataset = dataset
        self.transform = transform

    @lru_cache(maxsize=None)
    def _cached_transform(self, idx):
        x, _ = self.dataset[idx]
        return self.transform(x)

    def __len__(self):
        return len(self.dataset)

    def __getitem__(self, idx):
        _, y = self.dataset[idx]
        return self._cached_transform(idx), y