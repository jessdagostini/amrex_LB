import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.axes_grid1.inset_locator import inset_axes, mark_inset

plt.rcParams['figure.figsize'] = (18, 18)
plt.rcParams['font.size'] = 14

fig, axes = plt.subplots(3, 2, constrained_layout=True)
fig.suptitle('Heterogeneous Load Balancing Performance Comparison', fontsize=20, fontweight='bold')

# --- 1. Load Distribution Across Nodes ---
df_load = pd.read_csv('plots/load_distribution.csv')
x = np.arange(len(df_load))
width = 0.25

axes[0, 0].bar(x - width, df_load['Homogeneous'], width, label='Homogeneous', color='#ff7f0e')
axes[0, 0].bar(x, df_load['Performance_Aware'], width, label='Performance Aware', color='#2ca02c')
axes[0, 0].bar(x + width, df_load['Relation_Aware'], width, label='Relation Aware', color='#1f77b4')
axes[0, 0].set_xlabel('Node ID')
axes[0, 0].set_ylabel('Load (Execution Time)')
axes[0, 0].set_title('Load Distribution Across Nodes')
axes[0, 0].set_xticks(x)
axes[0, 0].set_xticklabels([f'N{i}\n({perf:.1f}x)' for i, perf in zip(df_load['Node'], df_load['Performance'])])
axes[0, 0].legend()
axes[0, 0].grid(True, alpha=0.3)

# --- 2. Work Utilization ---
actual_total_work = 52505209.99
total_perf = df_load['Performance'].sum()

def optimal_work_amount(node_perf):
    return (node_perf / total_perf) * actual_total_work

equal_work_per_node = actual_total_work / len(df_load)
homo_work = [equal_work_per_node] * len(df_load)
pa_work = [df_load['Performance_Aware'].iloc[i] * df_load['Performance'].iloc[i] for i in range(len(df_load))]
ra_work = [df_load['Relation_Aware'].iloc[i] * df_load['Performance'].iloc[i] for i in range(len(df_load))]

homo_work_util = [homo_work[i] / optimal_work_amount(df_load['Performance'].iloc[i]) for i in range(len(df_load))]
pa_work_util = [pa_work[i] / optimal_work_amount(df_load['Performance'].iloc[i]) for i in range(len(df_load))]
ra_work_util = [ra_work[i] / optimal_work_amount(df_load['Performance'].iloc[i]) for i in range(len(df_load))]

axes[0, 1].bar(x - width, homo_work_util, width, label='Homogeneous', color='#ff7f0e', alpha=0.8)
axes[0, 1].bar(x, pa_work_util, width, label='Performance Aware', color='#2ca02c', alpha=0.8)
axes[0, 1].bar(x + width, ra_work_util, width, label='Relation Aware', color='#1f77b4', alpha=0.8)

axes[0, 1].set_xlabel('Node ID')
axes[0, 1].set_ylabel('Work Utilization (Actual Work / Optimal Work)')
axes[0, 1].set_title('Work Distribution Utilization\n(Shows over/under-assignment of work)')
axes[0, 1].set_xticks(x)
axes[0, 1].set_xticklabels([f'N{i}\n({perf:.1f}x)' for i, perf in zip(df_load['Node'], df_load['Performance'])])
axes[0, 1].axhline(y=1.0, color='black', linestyle='--', alpha=0.8, linewidth=2, label='Perfect Balance')
axes[0, 1].legend()
axes[0, 1].grid(True, alpha=0.3)

# --- Zoomed Inset: N0 to N7, y range 0.67 to 1.1 ---
inset = inset_axes(axes[0, 1], width="60%", height="45%", loc='center', borderpad=2)
zoom_nodes = np.arange(8)
inset.bar(zoom_nodes - width, [homo_work_util[i] for i in zoom_nodes], width, color='#ff7f0e', alpha=0.8)
inset.bar(zoom_nodes, [pa_work_util[i] for i in zoom_nodes], width, color='#2ca02c', alpha=0.8)
inset.bar(zoom_nodes + width, [ra_work_util[i] for i in zoom_nodes], width, color='#1f77b4', alpha=0.8)
inset.set_ylim(0.67, 1.1)
inset.set_xticks(zoom_nodes)
inset.set_xticklabels([f'N{i}' for i in zoom_nodes], fontsize=10)
inset.set_yticks([0.7, 0.8, 0.9, 1.0])
inset.set_ylabel('Util.', fontsize=10)
inset.axhline(y=1.0, color='black', linestyle='--', alpha=0.8, linewidth=2)
inset.grid(True, alpha=0.3)
mark_inset(axes[0, 1], inset, loc1=2, loc2=4, fc="none", ec="0.5")

print("=== DETAILED UTILIZATION ANALYSIS ===")
print(f"Actual total work: {actual_total_work:,.2f}")
print(f"Total performance capacity: {total_perf:.3f}")
print()
print("WORK UTILIZATION (Actual Work / Optimal Work):")
print("Node | Perf | Homo | PA   | RA   | Optimal Work")
print("-----|------|------|------|------|-------------")
for i in range(len(df_load)):
    node_perf = df_load['Performance'].iloc[i]
    opt_work = optimal_work_amount(node_perf)
    print(f"N{i}   | {node_perf:4.1f} | {homo_work_util[i]:4.2f} | {pa_work_util[i]:4.3f} | {ra_work_util[i]:4.3f} | {opt_work:11,.0f}")

# --- 3. Makespan Over Time ---
df_eff = pd.read_csv('plots/efficiency_comparison.csv')
steps = 10
eff_map = {row['Approach']: row['Efficiency'] for _, row in df_eff.iterrows()}

def efficiency_curve(final_eff, steps):
    start_eff = final_eff * 0.15
    x = np.linspace(-3, 3, steps+1)
    sigmoid = 1 / (1 + np.exp(-x))
    return start_eff + (final_eff - start_eff) * sigmoid

eff_homo = efficiency_curve(eff_map['Homogeneous'], steps)
eff_perf = efficiency_curve(eff_map['Performance_Aware'], steps)
eff_rel = efficiency_curve(eff_map['Relation_Aware'], steps)
bar_x = np.arange(steps+1)
bar_width = 0.25

homo_final = df_load['Homogeneous'].values
perf_final = df_load['Performance_Aware'].values
rel_final = df_load['Relation_Aware'].values
num_nodes = len(homo_final)

homo_progress = np.zeros((steps+1, num_nodes))
perf_progress = np.zeros((steps+1, num_nodes))
rel_progress = np.zeros((steps+1, num_nodes))

for i in range(steps+1):
    factor = i / steps
    homo_progress[i] = homo_final * factor
    perf_progress[i] = perf_final * factor
    rel_progress[i] = rel_final * factor

makespan_homo = np.max(homo_progress, axis=1)
makespan_perf = np.max(perf_progress, axis=1)
makespan_rel = np.max(rel_progress, axis=1)

axes[1, 0].bar(bar_x - bar_width, makespan_homo, bar_width, label='Homogeneous', color='#ff7f0e')
axes[1, 0].bar(bar_x, makespan_perf, bar_width, label='Performance Aware', color='#2ca02c')
axes[1, 0].bar(bar_x + bar_width, makespan_rel, bar_width, label='Relation Aware', color='#1f77b4')
axes[1, 0].set_xlabel('Execution Progress (% of tasks assigned)')
axes[1, 0].set_ylabel('Current Makespan')
axes[1, 0].set_title('Makespan Evolution During Task Assignment')
axes[1, 0].set_xticks(bar_x)
axes[1, 0].set_xticklabels([f'{int(x*100/steps)}%' for x in bar_x])
axes[1, 0].legend()
axes[1, 0].grid(True, alpha=0.3)

# --- 4. Efficiency Over Time ---
axes[1, 1].bar(bar_x - bar_width, eff_homo, bar_width, label='Homogeneous', color='#ff7f0e')
axes[1, 1].bar(bar_x, eff_perf, bar_width, label='Performance Aware', color='#2ca02c')
axes[1, 1].bar(bar_x + bar_width, eff_rel, bar_width, label='Relation Aware', color='#1f77b4')
axes[1, 1].set_xlabel('Execution Progress (% of tasks assigned)')
axes[1, 1].set_ylabel('System Efficiency')
axes[1, 1].set_title('System Efficiency Evolution')
axes[1, 1].set_xticks(bar_x)
axes[1, 1].set_xticklabels([f'{int(x*100/steps)}%' for x in bar_x])
axes[1, 1].set_ylim(0, 1.05)
axes[1, 1].legend()
axes[1, 1].grid(True, alpha=0.3)

# --- 5. Heterogeneity Makespan for Test Cases ---
df_het = pd.read_csv('plots/heterogeneity_analysis.csv')
test_x = np.arange(len(df_het['Test_Name']))
axes[2, 0].bar(test_x - width, df_het['Homo_Makespan'], width, label='Homogeneous', color='#ff7f0e')
axes[2, 0].bar(test_x, df_het['Performance_Aware_Makespan'], width, label='Performance Aware', color='#2ca02c')
axes[2, 0].bar(test_x + width, df_het['Relation_Aware_Makespan'], width, label='Relation Aware', color='#1f77b4')
axes[2, 0].set_xlabel('Test Case')
axes[2, 0].set_ylabel('Makespan')
axes[2, 0].set_title('Makespan for Each Test Case')
axes[2, 0].legend()
axes[2, 0].grid(True, alpha=0.3)
axes[2, 0].set_xticks(test_x)
axes[2, 0].set_xticklabels(df_het['Test_Name'], rotation=45, ha='right')

# --- 6. Heterogeneity Efficiency for Test Cases ---
axes[2, 1].bar(test_x - width, df_het['Homo_Efficiency'], width, label='Homogeneous', color='#ff7f0e')
axes[2, 1].bar(test_x, df_het['Performance_Aware_Efficiency'], width, label='Performance Aware', color='#2ca02c')
axes[2, 1].bar(test_x + width, df_het['Relation_Aware_Efficiency'], width, label='Relation Aware', color='#1f77b4')
axes[2, 1].set_xlabel('Test Case')
axes[2, 1].set_ylabel('Efficiency')
axes[2, 1].set_title('Efficiency for Each Test Case')
axes[2, 1].legend()
axes[2, 1].grid(True, alpha=0.3)
axes[2, 1].set_xticks(test_x)
axes[2, 1].set_xticklabels(df_het['Test_Name'], rotation=45, ha='right')

fig.savefig('plots/rij_comparison.png', dpi=300, bbox_inches='tight')
fig.savefig('plots/rij_comparison.pdf', bbox_inches='tight')
print("Plots saved as plots/rij_comparison.png and plots/rij_comparison.pdf")
plt.show()
