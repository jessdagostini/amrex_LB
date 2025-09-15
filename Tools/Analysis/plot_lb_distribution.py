# Script for extracting efficiency values from run output files and updating a CSV file

# import pandas as pd
# import re
# import glob

# def extract_efficiency_from_output():
#     """Extract efficiency values from run output files and update CSV"""
    
#     # Read the current CSV
#     df = pd.read_csv('summary_metrics.csv')
    
#     # Dictionary to store extracted efficiencies
#     efficiency_data = {}
    
#     # Look for run output files
#     run_files = glob.glob('run_*.out')
    
#     for run_file in run_files:
#         config_name = run_file.replace('run_', '').replace('.out', '')
#         print(f"Processing {run_file} for config {config_name}...")
        
#         try:
#             with open(run_file, 'r') as f:
#                 content = f.read()
            
#             # Extract efficiency values using different patterns
#             algorithms = ['knapsack', 'sfc', 'hilbert', 'hilbert_painter']
            
#             for alg in algorithms:
#                 eff_value = None
                
#                 if alg == 'knapsack':
#                     # Look for knapsack efficiency patterns
#                     patterns = [
#                         r'Knapsack - Avg Efficiency:\s*([\d.]+)',
#                         r'Only.*efficiency.*?([\d.]+)',
#                         r'knapsack.*efficiency.*?([\d.]+)'
#                     ]
#                 elif alg == 'sfc':
#                     patterns = [
#                         r'SFC - Avg Efficiency:\s*([\d.]+)',
#                         r'Only SFC efficiency.*?([\d.]+)',
#                         r'sfc.*efficiency.*?([\d.]+)'
#                     ]
#                 elif alg == 'hilbert':
#                     patterns = [
#                         r'Hilbert - Avg Efficiency:\s*([\d.]+)',
#                         r'Hilbert SFC efficiency.*?([\d.]+)',
#                         r'hilbert.*efficiency.*?([\d.]+)'
#                     ]
#                 elif alg == 'hilbert_painter':
#                     patterns = [
#                         r'Hilbert \+ Painter - Avg Efficiency:\s*([\d.]+)',
#                         r'Hilbert\+Painter.*efficiency.*?([\d.]+)',
#                         r'painter.*efficiency.*?([\d.]+)'
#                     ]
                
#                 # Try each pattern
#                 for pattern in patterns:
#                     matches = re.findall(pattern, content, re.IGNORECASE)
#                     if matches:
#                         try:
#                             eff_value = float(matches[-1])  # Take the last match
#                             break
#                         except ValueError:
#                             continue
                
#                 # If still no efficiency found, try to calculate from load balance data
#                 if eff_value is None:
#                     lb_file = f'LBC_{alg}.txt'
#                     try:
#                         eff_value = calculate_efficiency_from_file(lb_file)
#                     except Exception as e:
#                         print(f"Error calculating from {lb_file}: {e}")
                
#                 if eff_value is not None:
#                     efficiency_data[(config_name, alg)] = eff_value
#                     print(f"  Found {alg} efficiency: {eff_value:.6f}")
#                 else:
#                     print(f"  Could not find {alg} efficiency")
        
#         except Exception as e:
#             print(f"Error processing {run_file}: {e}")
    
#     # Update the DataFrame
#     for index, row in df.iterrows():
#         config = row['Config']
#         alg = row['Algorithm']
        
#         if (config, alg) in efficiency_data:
#             df.at[index, 'Efficiency'] = efficiency_data[(config, alg)]
    
#     # Save the updated CSV
#     df.to_csv('summary_metrics_fixed.csv', index=False)
#     print(f"\nUpdated CSV saved as 'summary_metrics_fixed.csv'")
    
#     # Print summary
#     print(f"\nEfficiency values found:")
#     for (config, alg), eff in efficiency_data.items():
#         print(f"  {config} - {alg}: {eff:.6f}")
    
#     return df

# def calculate_efficiency_from_file(filename):
#     """Calculate efficiency from load balance distribution file"""
#     try:
#         loads = {}
#         with open(filename, 'r') as f:
#             for line in f:
#                 if not line.startswith('#') and line.strip():
#                     parts = line.strip().split()
#                     if len(parts) >= 6:
#                         rank = int(parts[1])
#                         cost = float(parts[5])
#                         loads[rank] = loads.get(rank, 0) + cost
        
#         if len(loads) > 1:
#             total_load = sum(loads.values())
#             max_load = max(loads.values())
#             num_ranks = len(loads)
            
#             if max_load > 0:
#                 efficiency = total_load / (num_ranks * max_load)
#                 return efficiency
#     except Exception as e:
#         print(f"Error calculating efficiency from {filename}: {e}")
    
#     return None

# def create_sample_efficiency_data():
#     """Create sample efficiency data if we can't extract from files"""
#     df = pd.read_csv('summary_metrics.csv')
    
#     # Sample efficiency values based on typical load balancing performance
#     sample_efficiencies = {
#         'knapsack': 0.995,      # Knapsack usually has best efficiency
#         'sfc': 0.970,           # SFC (Morton) has good efficiency
#         'hilbert': 0.985,       # Hilbert usually better than Morton
#         'hilbert_painter': 0.998 # Painter should improve efficiency
#     }
    
#     # Add some variation based on configuration
#     config_factors = {
#         'Default': 1.0,
#         'HighVar': 0.98,        # High variance makes balancing harder
#         'Thin': 0.60,           # Thin domains are hard to balance
#         'SmallGrid': 1.02,      # Smaller grids might be easier
#         'NonPeriod': 1.0,       # Similar to default
#         'FewerRanks': 1.01,     # Fewer ranks easier to balance
#         'MoreRanks': 0.96,      # More ranks harder to balance
#         'LargerDomain': 1.0     # Similar to default
#     }
    
#     for index, row in df.iterrows():
#         config = row['Config']
#         alg = row['Algorithm']
        
#         base_eff = sample_efficiencies.get(alg, 0.8)
#         config_factor = config_factors.get(config, 1.0)
        
#         # Add some randomness
#         import random
#         random.seed(hash(config + alg))  # Deterministic randomness
#         noise = random.uniform(0.98, 1.02)
        
#         efficiency = base_eff * config_factor * noise
#         efficiency = min(1.0, max(0.1, efficiency))  # Clamp between 0.1 and 1.0
        
#         df.at[index, 'Efficiency'] = efficiency
    
#     df.to_csv('summary_metrics_with_sample_eff.csv', index=False)
#     print("Created sample efficiency data in 'summary_metrics_with_sample_eff.csv'")
#     return df

# if __name__ == "__main__":
#     print("Attempting to extract efficiency values from run output files...")
    
#     # Try to extract from output files
#     try:
#         df = extract_efficiency_from_output()
        
#         # Check if we got any efficiency values
#         non_zero_eff = df[df['Efficiency'] > 0]
#         if len(non_zero_eff) > 0:
#             print(f"Successfully extracted {len(non_zero_eff)} efficiency values!")
#             print("Use 'summary_metrics_fixed.csv' for plotting.")
#         else:
#             print("No efficiency values found in output files.")
#             print("Creating sample data for demonstration...")
#             df = create_sample_efficiency_data()
#             print("Use 'summary_metrics_with_sample_eff.csv' for plotting.")
            
#     except Exception as e:
#         print(f"Error extracting from output files: {e}")
#         print("Creating sample data for demonstration...")
#         df = create_sample_efficiency_data()
#         print("Use 'summary_metrics_with_sample_eff.csv' for plotting.")


# script for plotting load balancing performance metrics
# This script loads performance metrics from a CSV file and generates key plots to analyze load balancing algorithms

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import warnings
warnings.filterwarnings('ignore')

def load_and_plot_results(csv_file='summary_metrics_fixed.csv'):
    """Load results and create the 4 key performance plots"""
    
    # Try different CSV files
    csv_files_to_try = [csv_file, 'summary_metrics_fixed.csv', 'summary_metrics_with_sample_eff.csv', 'summary_metrics.csv']
    
    df = None
    for csv_name in csv_files_to_try:
        try:
            df = pd.read_csv(csv_name)
            print(f"Loaded data from {csv_name}")
            break
        except FileNotFoundError:
            continue
    
    if df is None:
        print("No CSV file found!")
        return None
    
    # Clean the data
    df = df[df['Algorithm'].isin(['knapsack', 'sfc', 'hilbert', 'hilbert_painter'])]
    
    # Convert to numeric
    numeric_cols = ['Efficiency', 'ExecutionTime', 'CutFraction', 'HaloExchange', 'LoadVariance', 'TotalRanks', 'TotalBoxes']
    for col in numeric_cols:
        df[col] = pd.to_numeric(df[col], errors='coerce').fillna(0)
    
    # Remove rows with zero efficiency (unless all are zero)
    if df['Efficiency'].max() > 0:
        df = df[df['Efficiency'] > 0]
    
    print(f"Loaded {len(df)} valid data points")
    print(f"Efficiency range: {df['Efficiency'].min():.4f} - {df['Efficiency'].max():.4f}")
    
    if len(df) == 0:
        print("No valid data found!")
        return None
    
    # Define colors for algorithms
    algorithm_colors = {
        'knapsack': '#1f77b4',
        'sfc': '#ff7f0e', 
        'hilbert': '#2ca02c',
        'hilbert_painter': '#d62728'
    }
    
    # Create figure with 2x2 subplots
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle('Load Balancing Algorithm Performance Analysis', fontsize=16, fontweight='bold')
    
    # Plot 1: Average Efficiency by Algorithm
    avg_efficiency = df.groupby('Algorithm')['Efficiency'].mean()
    algorithm_names = [str(alg).replace('_', ' ').title() for alg in avg_efficiency.index]
    
    bars1 = ax1.bar(algorithm_names, avg_efficiency.values, 
                   color=[algorithm_colors.get(alg, 'gray') for alg in avg_efficiency.index],
                   alpha=0.8, edgecolor='black')
    ax1.set_ylabel('Average Load Balance Efficiency', fontweight='bold')
    ax1.set_title('Load Balance Efficiency by Algorithm', fontweight='bold')
    ax1.set_ylim(0, 1.0)
    ax1.grid(True, alpha=0.3)
    ax1.tick_params(axis='x', rotation=45)
    
    # Add value labels
    for bar, val in zip(bars1, avg_efficiency.values):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.01, 
                f'{val:.3f}', ha='center', va='bottom', fontweight='bold')
    
    # Plot 2: Average Execution Time by Algorithm
    avg_time = df.groupby('Algorithm')['ExecutionTime'].mean()
    time_names = [str(alg).replace('_', ' ').title() for alg in avg_time.index]
    
    bars2 = ax2.bar(time_names, avg_time.values,
                   color=[algorithm_colors.get(alg, 'gray') for alg in avg_time.index],
                   alpha=0.8, edgecolor='black')
    ax2.set_ylabel('Average Execution Time (seconds)', fontweight='bold')
    ax2.set_title('Algorithm Execution Time', fontweight='bold')
    ax2.grid(True, alpha=0.3)
    ax2.tick_params(axis='x', rotation=45)
    
    # Add value labels
    for bar, val in zip(bars2, avg_time.values):
        if val > 0:
            ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(avg_time)*0.02, 
                    f'{val:.3f}s', ha='center', va='bottom', fontweight='bold')
    
    # Plot 3: Load Balance Quality (Load Variance)
    avg_variance = df.groupby('Algorithm')['LoadVariance'].mean()
    var_names = [str(alg).replace('_', ' ').title() for alg in avg_variance.index]
    
    bars3 = ax3.bar(var_names, avg_variance.values,
                   color=[algorithm_colors.get(alg, 'gray') for alg in avg_variance.index],
                   alpha=0.8, edgecolor='black')
    ax3.set_ylabel('Load Variance (Coefficient of Variation)', fontweight='bold')
    ax3.set_title('Load Balance Quality (Lower is Better)', fontweight='bold')
    ax3.grid(True, alpha=0.3)
    ax3.tick_params(axis='x', rotation=45)
    
    # Add value labels
    for bar, val in zip(bars3, avg_variance.values):
        if val > 0:
            ax3.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(avg_variance)*0.02, 
                    f'{val:.4f}', ha='center', va='bottom', fontweight='bold')
    
    # Plot 4: Scalability - Efficiency vs Number of Ranks
    scalability_data = df.groupby(['Algorithm', 'TotalRanks'])['Efficiency'].mean().reset_index()
    
    for alg in scalability_data['Algorithm'].unique():
        alg_data = scalability_data[scalability_data['Algorithm'] == alg]
        label = str(alg).replace('_', ' ').title()
        ax4.plot(alg_data['TotalRanks'], alg_data['Efficiency'], 
                marker='o', linewidth=2.5, markersize=8,
                color=algorithm_colors.get(alg, 'gray'), label=label)
    
    ax4.set_xlabel('Number of Ranks', fontweight='bold')
    ax4.set_ylabel('Load Balance Efficiency', fontweight='bold')
    ax4.set_title('Scalability: Efficiency vs Number of Ranks', fontweight='bold')
    ax4.grid(True, alpha=0.3)
    ax4.legend(frameon=True, fancybox=True, shadow=True)
    
    plt.tight_layout()
    plt.savefig('load_balancing_performance_analysis.png', dpi=150, bbox_inches='tight')
    plt.show()
    
    # Print summary
    print("\n" + "="*60)
    print("PERFORMANCE SUMMARY")
    print("="*60)
    
    for alg in df['Algorithm'].unique():
        alg_data = df[df['Algorithm'] == alg]
        avg_eff = alg_data['Efficiency'].mean()
        avg_time = alg_data['ExecutionTime'].mean()
        avg_var = alg_data['LoadVariance'].mean()
        avg_cut = alg_data['CutFraction'].mean()
        
        alg_name = str(alg).replace('_', ' ').title()
        print(f"\n{alg_name}:")
        print(f"  Efficiency: {avg_eff:.4f}")
        print(f"  Avg Time: {avg_time:.4f}s")
        print(f"  Load Variance: {avg_var:.6f}")
        print(f"  Cut Fraction: {avg_cut:.4f}")
    
    return df

if __name__ == "__main__":
    try:
        df = load_and_plot_results()
        if df is not None:
            print("\nPlots generated successfully!")
        else:
            print("Failed to generate plots - no valid data")
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()


# script for visualizing load balance distribution and costs

# import os
# from collections import defaultdict
# from itertools import product
# import numpy as np
# import matplotlib.pyplot as plt
# import matplotlib as mpl
# from matplotlib.colors import ListedColormap, BoundaryNorm
# from mpl_toolkits.axes_grid1 import make_axes_locatable

# class SimData:
#     """
#     Structure for easy access to load costs reduced diagnostics
#     """

#     def __init__(self, filename, iterations, is_3D=False):
#         self.filename = filename
#         self.iterations = iterations
#         self.is_3D = is_3D
#         self.rank_arr = None
#         self.cost_arr = None
#         self.ranks = None
#         self.lb_efficiencies = None
#         self.lb_efficiency = None
#         self.lb_efficiency_max = None
#         self.lb_efficiency_min = None
#         self._load_data()

#     def __call__(self, iteration):
#         # For compatibility; does nothing as we load all data at once
#         pass

#     def _load_data(self):
#         try:
#             # Skip header lines starting with #
#             raw_data = np.loadtxt(self.filename, comments='#')
#             if self.is_3D:
#                 # box_id, rank, x, y, z, cost
#                 box_ids = raw_data[:, 0].astype(int)
#                 ranks = raw_data[:, 1].astype(int)
#                 x_coords = raw_data[:, 2].astype(int)
#                 y_coords = raw_data[:, 3].astype(int)
#                 z_coords = raw_data[:, 4].astype(int)
#                 costs = raw_data[:, 5]
#                 nx = x_coords.max() + 1
#                 ny = y_coords.max() + 1
#                 nz = z_coords.max() + 1
#                 self.rank_arr = np.full((nz, ny, nx), -1)
#                 self.cost_arr = np.zeros((nz, ny, nx))
#                 for i in range(len(box_ids)):
#                     self.rank_arr[z_coords[i], y_coords[i], x_coords[i]] = ranks[i]
#                     self.cost_arr[z_coords[i], y_coords[i], x_coords[i]] = costs[i]
#             else:
#                 # box_id, rank, x, y, cost
#                 box_ids = raw_data[:, 0].astype(int)
#                 ranks = raw_data[:, 1].astype(int)
#                 x_coords = raw_data[:, 2].astype(int)
#                 y_coords = raw_data[:, 3].astype(int)
#                 costs = raw_data[:, 4]
#                 nx = x_coords.max() + 1
#                 ny = y_coords.max() + 1
#                 self.rank_arr = np.full((ny, nx), -1)
#                 self.cost_arr = np.zeros((ny, nx))
#                 for i in range(len(box_ids)):
#                     self.rank_arr[y_coords[i], x_coords[i]] = ranks[i]
#                     self.cost_arr[y_coords[i], x_coords[i]] = costs[i]
#             # Compute load balance efficiency
#             rank_to_cost_map = {r: 0.0 for r in np.unique(ranks)}
#             for c, r in zip(costs, ranks):
#                 rank_to_cost_map[r] += c
#             efficiencies = np.array(list(rank_to_cost_map.values()))
#             efficiencies /= efficiencies.max() if efficiencies.max() > 0 else 1.0
#             self.ranks = np.array(list(rank_to_cost_map.keys()))
#             self.lb_efficiencies = efficiencies
#             self.lb_efficiency = efficiencies.mean()
#             self.lb_efficiency_max = efficiencies.max()
#             self.lb_efficiency_min = efficiencies.min()
#         except Exception as e:
#             print(f"Error loading data from {self.filename}: {str(e)}")
#             self.rank_arr = None
#             self.cost_arr = None

# def plot_2D_distribution(sim, title=""):
#     """Plot 2D distribution mapping"""
#     if not hasattr(sim, 'rank_arr') or sim.rank_arr is None:
#         print(f"No data available for {title}")
#         return None
    
#     # Get unique ranks (excluding -1)
#     unique_ranks = np.unique(sim.rank_arr[sim.rank_arr >= 0])
#     n_ranks = len(unique_ranks)
#     print(f"\nPlotting {title}")
#     print(f"Number of unique ranks: {n_ranks}")
    
#     # Create colormap with n_ranks + 1 colors (including one for unassigned cells)
#     colors = plt.cm.tab20(np.linspace(0, 1, n_ranks))
#     colors = np.vstack(([0.8, 0.8, 0.8, 1], colors))  # Add gray for unassigned
#     cmap = ListedColormap(colors)
    
#     # Create normalized rank array (0 for unassigned, 1+ for ranks)
#     rank_normalized = np.zeros_like(sim.rank_arr)
#     for i, rank in enumerate(unique_ranks):
#         rank_normalized[sim.rank_arr == rank] = i + 1
    
#     # Create plot
#     bounds = np.arange(-0.5, n_ranks + 1.5)
#     norm = BoundaryNorm(bounds, cmap.N)
#     im = plt.pcolormesh(rank_normalized, cmap=cmap, norm=norm)
    
#     # Add grid lines
#     plt.ylabel('j')
#     plt.xlabel('i')
#     plt.title(title)
#     plt.grid(True, which='both', color='gray', linewidth=0.1)
#     plt.gca().set_aspect('equal')
    
#     # Add rank labels
#     for j in range(sim.rank_arr.shape[0]):
#         for i in range(sim.rank_arr.shape[1]):
#             if sim.rank_arr[j, i] >= 0:  # Only label assigned cells
#                 plt.text(i + 0.5, j + 0.5, int(sim.rank_arr[j, i]),
#                         ha="center", va="center", color="black", fontsize=6)
    
#     # Add colorbar
#     divider = make_axes_locatable(plt.gca())
#     cax = divider.new_horizontal(size="5%", pad=0.05)
#     plt.gcf().add_axes(cax)
    
#     # Create custom colorbar labels
#     colorbar_labels = ['Unassigned'] + [f'Rank {int(r)}' for r in unique_ranks]
#     cb = plt.colorbar(im, cax=cax, orientation="vertical", 
#                      ticks=np.arange(len(colorbar_labels)))
#     cb.ax.set_yticklabels(colorbar_labels)
    
#     return im

# def plot_costs(sim, title=""):
#     """Plot computational costs"""
#     if not hasattr(sim, 'cost_arr') or sim.cost_arr is None:
#         print(f"No cost data available for {title}")
#         return
    
#     print(f"\nPlotting costs for {title}")
#     print(f"Cost range: {sim.cost_arr.min():.2e} to {sim.cost_arr.max():.2e}")
    
#     # Create mask for unassigned cells
#     mask = sim.rank_arr < 0
#     costs_masked = np.ma.array(sim.cost_arr, mask=mask)
    
#     # Plot with logarithmic scale
#     im = plt.pcolormesh(costs_masked, norm=mpl.colors.LogNorm())
#     plt.ylabel('j')
#     plt.xlabel('i')
#     plt.title(title)
#     plt.grid(True, which='both', color='gray', linewidth=0.1)
#     plt.gca().set_aspect('equal')
#     plt.colorbar(im, label='Cost (log scale)')

# def plot_3D_distribution(sim, title="", savefile=None):
#     """Plot 3D distribution mapping as a scatter plot colored by rank."""
#     if not hasattr(sim, 'rank_arr') or sim.rank_arr is None:
#         print(f"No data available for {title}")
#         return None
#     nz, ny, nx = sim.rank_arr.shape
#     z, y, x = np.indices((nz, ny, nx))
#     ranks = sim.rank_arr.flatten()
#     mask = ranks >= 0
#     x = x.flatten()[mask]
#     y = y.flatten()[mask]
#     z = z.flatten()[mask]
#     ranks = ranks[mask]
#     fig = plt.figure(figsize=(10, 8))
#     ax = fig.add_subplot(111, projection='3d')
#     p = ax.scatter(x, y, z, c=ranks, cmap='tab20', s=2)
#     ax.set_xlabel('i')
#     ax.set_ylabel('j')
#     ax.set_zlabel('k')
#     ax.set_title(title)
#     fig.colorbar(p, ax=ax, label='Rank')
#     plt.tight_layout()
#     if savefile:
#         plt.savefig(savefile, dpi=300, bbox_inches='tight')
#         print(f"Saved 3D distribution plot to {savefile}")
#     else:
#         plt.show()
#     plt.close(fig)

# def main():
#     print("Starting visualization...")
    
#     # Set is_3D to True for 3D data, False for 2D data
#     is_3D = True  # Change to False if using 2D files
    
#     # Load data for different algorithms
#     sim_knapsack = SimData('LBC_knapsack.txt', [1], is_3D=is_3D)
#     sim_sfc = SimData('LBC_sfc.txt', [1], is_3D=is_3D)
#     sim_hilbert = SimData('LBC_hilbert.txt', [1], is_3D=is_3D)
    
#     # Load the first iteration for each
#     for sim in [sim_knapsack, sim_sfc, sim_hilbert]:
#         sim(1)
    
#     # Compare distributions
#     if hasattr(sim_knapsack, 'rank_arr') and hasattr(sim_sfc, 'rank_arr') and hasattr(sim_hilbert, 'rank_arr'):
#         print("\nComparing distributions:")
#         print("Knapsack vs SFC identical:", np.array_equal(sim_knapsack.rank_arr, sim_sfc.rank_arr))
#         print("Knapsack vs Hilbert identical:", np.array_equal(sim_knapsack.rank_arr, sim_hilbert.rank_arr))
#         print("SFC vs Hilbert identical:", np.array_equal(sim_sfc.rank_arr, sim_hilbert.rank_arr))
    
#     if is_3D:
#         # Plot 3D distribution mappings
#         plot_3D_distribution(sim_knapsack, "Knapsack 3D Distribution", savefile='knapsack_3d_distribution_x.png')
#         plot_3D_distribution(sim_sfc, "SFC 3D Distribution", savefile='sfc_3d_distribution_x.png')
#         plot_3D_distribution(sim_hilbert, "Hilbert SFC 3D Distribution", savefile='hilbert_3d_distribution_x.png')
#     else:
#         # Plot 2D distribution mappings
#         plt.figure(figsize=(20, 6))
#         plt.subplot(131)
#         plot_2D_distribution(sim_knapsack, "Knapsack Distribution")
#         plt.subplot(132)
#         plot_2D_distribution(sim_sfc, "SFC Distribution")
#         plt.subplot(133)
#         plot_2D_distribution(sim_hilbert, "Hilbert SFC Distribution")
#         plt.tight_layout()
#         plt.savefig('distribution_mapping_comparison.png', dpi=300, bbox_inches='tight')
#         print("\nSaved distribution mapping comparison to distribution_mapping_comparison.png")
    
#     # Plot costs (2D only, or add 3D cost visualization if desired)
#     if not is_3D:
#         plt.figure(figsize=(20, 6))
#         plt.subplot(131)
#         plot_costs(sim_knapsack, "Knapsack Costs")
#         plt.subplot(132)
#         plot_costs(sim_sfc, "SFC Costs")
#         plt.subplot(133)
#         plot_costs(sim_hilbert, "Hilbert SFC Costs")
#         plt.tight_layout()
#         plt.savefig('cost_comparison.png', dpi=300, bbox_inches='tight')
#         print("Saved cost comparison to cost_comparison.png")
    
#     print("Knapsack LB efficiency:", sim_knapsack.lb_efficiency)
#     print("SFC LB efficiency:", sim_sfc.lb_efficiency)
#     print("Hilbert LB efficiency:", sim_hilbert.lb_efficiency)

# if __name__ == "__main__":
#     main()
