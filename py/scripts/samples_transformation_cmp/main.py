import torch
from torchvision.transforms import v2 as transforms
from torchvision import datasets
from common.transform.deskew.deskew import Deskew
from common.transform.soft_threshold import SoftThreshold
from common.transform.minst_transform import ResizeDeskewTransform, ResizeDeskewThresholdTransform
from common.util.output_path import output_path
from matplotlib import pyplot as plt


def show_samples(dataset, seed = 0, file_name=None):
    import random
    random.seed(seed)
    plt.figure(figsize=(12, 5))
    for i in range(10):
        label = None
        while label != i:
            idx = random.randint(0, len(dataset)-1)
            img, label = dataset[idx]
        plt.subplot(2, 5, i+1)
        plt.imshow(img.squeeze(), cmap='viridis', vmin=0, vmax=1)
        # plt.title(f'Etykieta: {label}', fontsize=15)
        plt.axis('off')
    plt.tight_layout()
    if file_name:
        plt.savefig(output_path(file_name))

if __name__ == '__main__':
    sample_seed = 999
    resize = (16,16)
    threshold = 0.4

    # Step 0: Original
    transform = transforms.Compose([
        transforms.ToImage(), 
        transforms.ToDtype(torch.float32, scale=True),
        ])
    train_dataset = datasets.MNIST(root='./data', train=True, transform=transform, download=True)
    show_samples(train_dataset, seed = sample_seed, file_name='samples_step0.pdf')

    # Step 1: Resize
    transform = transforms.Compose([
        transforms.Resize(resize, antialias=True, interpolation=transforms.InterpolationMode.BICUBIC),
        transforms.ToImage(), 
        transforms.ToDtype(torch.float32, scale=True),
        ])
    train_dataset = datasets.MNIST(root='./data', train=True, transform=transform, download=True)
    show_samples(train_dataset, seed = sample_seed, file_name='samples_step1_resize.pdf')

    # Step 2: Deskew
    transform = ResizeDeskewTransform(resize=resize)
    train_dataset = datasets.MNIST(root='./data', train=True, transform=transform, download=True)
    show_samples(train_dataset, seed = sample_seed, file_name='samples_step2_deskew.pdf')

    # Step 3: Soft Threshold
    transform = ResizeDeskewThresholdTransform(resize=resize, threshold=threshold)
    train_dataset = datasets.MNIST(root='./data', train=True, transform=transform, download=True)
    show_samples(train_dataset, seed = sample_seed, file_name='samples_step3_soft_threshold.pdf')

