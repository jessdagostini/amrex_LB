# Math
import numpy as np
import random
import os
import sys
from collections import defaultdict


# Read file from stdin
directory = sys.argv[1] if len(sys.argv) > 1 else 'LBC.txt'
# Get the range of steps to extract, from 0 to 2000 with increments of 100
prange = [i for i in range(0, 5001, 100)]
# print(prange)
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

# Collect the costs and ranks
for key in keys:
    row = np.where(key == steps)[0][0]
    costs = data[row, 0::n_data_fields].astype(float)
    ranks = data[row, 1::n_data_fields].astype(int)
    # print(f"Step: {key}, Costs shape: {costs.shape}, Ranks shape: {ranks.shape}")
    # with np.printoptions(threshold=np.inf):
    #     print(costs)
    #     print(ranks)

    # Print the list of costs and ranks
    # print("Step:", key)
    print(','.join(f"{cost:.2f}" for cost in costs.flatten()))
    print(','.join(str(rank) for rank in ranks.flatten()))
    # print()  # Blank line for separation