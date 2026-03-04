from matplotlib import pyplot as plt
from common.util.output_path import output_path

def plot_curve(values, xlabel, ylabel, title, file_name):
    plt.figure(figsize=(8, 5))
    x_values = list(range(1, len(values) + 1))
    plt.plot(x_values, values, linestyle='--', linewidth=1.0, marker='o', markersize=5)
    plt.grid(True, linestyle=':', alpha=0.6)
    x_ticks = list(range(1, len(values) + 2, max(1, len(values) // 10)))
    plt.xticks(x_ticks)
    plt.tick_params(direction="in")
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.tight_layout()
    plt.savefig(output_path(file_name))