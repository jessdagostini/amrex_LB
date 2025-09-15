import numpy as np
import random
import sys
from collections import defaultdict
import os

if len(sys.argv) < 2:
    print("Usage: python parse_lb_data.py <directory> [max_prange] [output_file]")
    exit(-1)

directory = sys.argv[1]
max_prange = int(sys.argv[2]) if len(sys.argv) > 2 else 2000
output_file = sys.argv[3] if len(sys.argv) > 3 else "output.txt"
# Get the range of steps to extract, from 0 to 2000 with increments of 100
prange = [i for i in range(0, max_prange+1, 100)]

if not os.path.exists(directory):
    print("Directory " + directory + " does not exist")
    exit(-1)

data_fields = defaultdict(dict)
data_fields, keys = data_fields, list(prange)

data = np.genfromtxt(directory)

if len(data.shape) == 1:
    data = data.reshape(-1, data.shape[0])

steps = data[:, 0].astype(int)

times = data[:, 1]
data = data[:, 2:]

# Compute the number of datafields saved per box
n_data_fields = 0
with open(directory) as f:
    h = f.readlines()[0]
    unique_headers = [
        "".join([ln for ln in w if not ln.isdigit()]) for w in h.split()
    ][2::]

# Either 9 or 10 depending if GPU
n_data_fields = 9 if len(set(unique_headers)) % 9 == 0 else 10
f.close()

# Collect the costs and ranks and write to output file
with open(output_file, 'w') as out_fh:
    for key in keys:
        row = np.where(key == steps)[0][0]
        costs = data[row, 0::n_data_fields].astype(float)
        ranks = data[row, 1::n_data_fields].astype(int)
        out_fh.write(','.join(f"{cost:.10f}" for cost in costs.flatten()) + '\n')
        out_fh.write(','.join(str(rank) for rank in ranks.flatten()) + '\n')
