import matplotlib.pyplot as plt
import os

def read_cut_fraction(filename):
    with open(filename) as f:
        for line in f:
            if line.strip().startswith('CutFraction'):
                return float(line.strip().split()[1])
    raise ValueError(f"CutFraction not found in {filename}")

def read_halo_volumes(filename):
    volumes = []
    with open(filename) as f:
        for line in f:
            if 'Halo exchange volume' in line:
                try:
                    volumes.append(int(line.strip().split()[-1]))
                except Exception:
                    pass
    if not volumes:
        raise ValueError(f"No halo exchange volumes found in {filename}")
    return volumes

# File names
algorithms = ['Knapsack', 'SFC', 'Hilbert']
cut_files = [
    'LBC_knapsack_graph_cut.txt',
    'LBC_sfc_graph_cut.txt',
    'LBC_hilbert_graph_cut.txt'
]
halo_files = [
    'LBC_knapsack_halo_exchange.txt',
    'LBC_sfc_halo_exchange.txt',
    'LBC_hilbert_halo_exchange.txt'
]

# Check files exist
for f in cut_files + halo_files:
    if not os.path.isfile(f):
        print(f"Warning: File {f} not found. Please check your filenames and working directory.")

# 1. Bar chart of graph cut fractions
cut_fractions = []
for f in cut_files:
    try:
        cut_fractions.append(read_cut_fraction(f))
    except Exception as e:
        print(e)
        cut_fractions.append(0.0)

plt.figure(figsize=(6,4))
plt.bar(algorithms, cut_fractions, color=['red', 'blue', 'green'])
plt.ylabel('Cut Fraction')
plt.title('Graph Cut Fraction by Algorithm')
plt.ylim(0, 1)
plt.tight_layout()
plt.savefig('graph_cut_fraction.png')
plt.show()

# 2. Bar chart of total halo exchange volume
total_volumes = []
for f in halo_files:
    try:
        total_volumes.append(sum(read_halo_volumes(f)))
    except Exception as e:
        print(e)
        total_volumes.append(0)

plt.figure(figsize=(6,4))
plt.bar(algorithms, total_volumes, color=['red', 'blue', 'green'])
plt.ylabel('Total Halo Exchange Volume')
plt.title('Total Halo Exchange by Algorithm')
plt.tight_layout()
plt.savefig('total_halo_exchange.png')
plt.show()

# 3. Boxplot of per-rank halo exchange volumes
all_volumes = []
for f in halo_files:
    try:
        all_volumes.append(read_halo_volumes(f))
    except Exception as e:
        print(e)
        all_volumes.append([0])

plt.figure(figsize=(6,4))
plt.boxplot(all_volumes, labels=algorithms)
plt.ylabel('Halo Exchange Volume')
plt.title('Halo Exchange Volume Distribution')
plt.tight_layout()
plt.savefig('halo_exchange_boxplot.png')
plt.show()