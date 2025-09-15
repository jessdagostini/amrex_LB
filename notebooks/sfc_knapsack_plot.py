import matplotlib.pyplot as plt
import numpy as np
from scipy.ndimage import zoom

np.random.seed(0)
data_knapsack = np.random.randint(0, 100, size=(50, 50))

def create_zorder_pattern(size, num_ranks):
    def bit_interleave(x, y):
        z = 0
        for i in range(int(np.log2(size))):
            z |= (x & (1 << i)) << i | (y & (1 << i)) << (i + 1)
        return z

    rank_array = np.zeros((size, size), dtype=int)
    ranks = np.arange(num_ranks)
    grid_size = size // int(np.sqrt(num_ranks))
    
    for i in range(0, size, grid_size):
        for j in range(0, size, grid_size):
            rank = bit_interleave(i // grid_size, j // grid_size) % num_ranks
            rank_array[i:i + grid_size, j:j + grid_size] = ranks[rank]
    
    return rank_array

size = 50
num_ranks = 100
data_zorder = create_zorder_pattern(size, num_ranks)

fig, ax = plt.subplots(1, 2, figsize=(15, 7))
cax1 = ax[0].matshow(data_knapsack, cmap='tab20c')
fig.colorbar(cax1, ax=ax[0], orientation='vertical')
ax[0].set_title('Knapsack', fontsize=20)
ax[0].set_xlabel('z [μm]', fontsize=25)
ax[0].set_ylabel('x [μm]', fontsize=20)
ax[0].tick_params(axis='both', which='major', labelsize=20)
ax[0].tick_params(axis='both', which='minor', labelsize=20)

cax2 = ax[1].matshow(data_zorder, cmap='tab20c')
fig.colorbar(cax2, ax=ax[1], orientation='vertical')
ax[1].set_title('Z-order space filling curve', fontsize=20)
ax[1].set_xlabel('z [μm]', fontsize=20)
ax[1].set_ylabel('x [μm]', fontsize=20)
ax[1].tick_params(axis='both', which='major', labelsize=20)
ax[1].tick_params(axis='both', which='minor', labelsize=20)

plt.tight_layout()
plt.savefig('knapsack_zorder_comparison.png', dpi=600)

plt.show()


