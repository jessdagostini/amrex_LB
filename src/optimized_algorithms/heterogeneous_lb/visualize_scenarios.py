import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

def load_and_analyze_comprehensive_data():
    """Load and analyze the comprehensive scenario data"""
    df = pd.read_csv('comprehensive_scenario_analysis.csv')
    
    # Create performance profile names
    perf_names = ['Moderate', 'High_Hetero', 'Homogeneous', 'Gradual', 'Mixed']
    preload_names = ['None', 'Node5_Heavy', 'Node0_Heavy', 'Node2_Heavy', 'First3_Heavy', 'Increasing']
    
    df['Perf_Name'] = df['Perf_Profile'].map(lambda x: perf_names[x])
    df['Preload_Name'] = df['Preload_Profile'].map(lambda x: preload_names[x])
    
    return df

def plot_strategy_comparison_heatmap(df):
    """Create heatmap showing which strategy wins under different conditions"""
    fig, axes = plt.subplots(2, 3, figsize=(18, 12))
    axes = axes.flatten()
    
    # Group by performance profile
    for i, perf_name in enumerate(['Moderate', 'High_Hetero', 'Homogeneous', 'Gradual', 'Mixed']):
        if i >= 6:
            break
            
        perf_data = df[df['Perf_Name'] == perf_name]
        
        # Create pivot table for heatmap
        pivot_data = perf_data.pivot_table(
            values='Winner', 
            index='Preload_Name', 
            columns='Task_Size',
            aggfunc=lambda x: (x == 'Knapsack').mean()
        )
        
        ax = axes[i]
        sns.heatmap(pivot_data, annot=True, fmt='.2f', cmap='RdYlBu_r', 
                   center=0.5, ax=ax, cbar_kws={'label': 'Knapsack Win Rate'})
        ax.set_title(f'{perf_name} Performance Profile\n(1.0 = Knapsack always wins, 0.0 = Greedy always wins)')
        ax.set_xlabel('Task Size')
        ax.set_ylabel('Preload Scenario')
    
    # Hide unused subplot
    if len(axes) > 5:
        axes[5].set_visible(False)
    
    plt.tight_layout()
    plt.savefig('strategy_comparison_heatmap.png', dpi=300, bbox_inches='tight')
    plt.show()

def plot_performance_differences(df):
    """Plot performance differences between strategies"""
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
    
    # Plot 1: Time difference distribution
    ax1.hist(df['Time_Diff'], bins=50, alpha=0.7, edgecolor='black')
    ax1.set_xlabel('Time Difference (|Greedy - Knapsack|)')
    ax1.set_ylabel('Frequency')
    ax1.set_title('Distribution of Performance Differences')
    ax1.axvline(df['Time_Diff'].mean(), color='red', linestyle='--', 
                label=f'Mean: {df["Time_Diff"].mean():.2f}')
    ax1.legend()
    
    # Plot 2: Performance by task size
    task_size_analysis = df.groupby('Task_Size').agg({
        'Time_Diff': 'mean',
        'Winner': lambda x: (x == 'Knapsack').mean()
    }).reset_index()
    
    ax2_twin = ax2.twinx()
    ax2.bar(task_size_analysis['Task_Size'], task_size_analysis['Time_Diff'], 
            alpha=0.7, color='skyblue', label='Avg Time Diff')
    ax2_twin.plot(task_size_analysis['Task_Size'], task_size_analysis['Winner'], 
                  'ro-', linewidth=2, label='Knapsack Win Rate')
    
    ax2.set_xlabel('Task Size')
    ax2.set_ylabel('Average Time Difference', color='blue')
    ax2_twin.set_ylabel('Knapsack Win Rate', color='red')
    ax2.set_title('Performance vs Task Size')
    ax2.legend(loc='upper left')
    ax2_twin.legend(loc='upper right')
    
    # Plot 3: Performance by heterogeneity
    hetero_analysis = df.groupby('Perf_Name').agg({
        'Time_Diff': 'mean',
        'Winner': lambda x: (x == 'Knapsack').mean(),
        'Greedy_Best_Time': 'mean',
        'Knap_Best_Time': 'mean'
    }).reset_index()
    
    x_pos = np.arange(len(hetero_analysis))
    width = 0.35
    
    ax3.bar(x_pos - width/2, hetero_analysis['Greedy_Best_Time'], width, 
            label='Greedy Avg Time', alpha=0.8, color='lightcoral')
    ax3.bar(x_pos + width/2, hetero_analysis['Knap_Best_Time'], width, 
            label='Knapsack Avg Time', alpha=0.8, color='lightgreen')
    
    ax3.set_xlabel('Performance Profile')
    ax3.set_ylabel('Average Best Time')
    ax3.set_title('Strategy Performance by Heterogeneity Level')
    ax3.set_xticks(x_pos)
    ax3.set_xticklabels(hetero_analysis['Perf_Name'], rotation=45)
    ax3.legend()
    
    # Plot 4: Node selection patterns
    node_selection = df.groupby(['Perf_Name', 'Greedy_Best_Node']).size().unstack(fill_value=0)
    node_selection_knap = df.groupby(['Perf_Name', 'Knap_Best_Node']).size().unstack(fill_value=0)
    
    # Normalize to percentages
    node_selection_pct = node_selection.div(node_selection.sum(axis=1), axis=0) * 100
    node_selection_knap_pct = node_selection_knap.div(node_selection_knap.sum(axis=1), axis=0) * 100
    
    # Plot greedy selections
    node_selection_pct.plot(kind='bar', stacked=True, ax=ax4, alpha=0.7)
    ax4.set_xlabel('Performance Profile')
    ax4.set_ylabel('Selection Percentage')
    ax4.set_title('Node Selection Patterns (Greedy Strategy)')
    ax4.legend(title='Node ID', bbox_to_anchor=(1.05, 1), loc='upper left')
    ax4.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig('performance_analysis.png', dpi=300, bbox_inches='tight')
    plt.show()

def plot_detailed_scenario(df_detail):
    """Plot detailed analysis of a single scenario"""
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 10))
    
    # Plot 1: Current loads comparison
    x_pos = np.arange(len(df_detail))
    width = 0.35
    
    ax1.bar(x_pos - width/2, df_detail['greedy_current'], width, 
            label='Greedy Current', alpha=0.8, color='lightblue')
    ax1.bar(x_pos + width/2, df_detail['knap_current'], width, 
            label='Knapsack Current', alpha=0.8, color='lightgreen')
    
    ax1.set_xlabel('Node ID')
    ax1.set_ylabel('Current Load')
    ax1.set_title('Current Load Distribution')
    ax1.set_xticks(x_pos)
    ax1.set_xticklabels([f'cpu{i}\n(perf={p:.1f})' for i, p in 
                        zip(df_detail['node_id'], df_detail['perf_factor'])])
    ax1.legend()
    
    # Plot 2: Projected loads comparison
    ax2.bar(x_pos - width/2, df_detail['greedy_proj'], width, 
            label='Greedy Projected', alpha=0.8, color='darkblue')
    ax2.bar(x_pos + width/2, df_detail['knap_proj'], width, 
            label='Knapsack Projected', alpha=0.8, color='darkgreen')
    
    # Highlight best choices
    greedy_best = df_detail['greedy_rank'].idxmin()
    knap_best = df_detail['knap_rank'].idxmin()
    
    ax2.bar(greedy_best - width/2, df_detail.loc[greedy_best, 'greedy_proj'], width,
            fill=False, edgecolor='red', linewidth=3)
    ax2.bar(knap_best + width/2, df_detail.loc[knap_best, 'knap_proj'], width,
            fill=False, edgecolor='red', linewidth=3)
    
    ax2.set_xlabel('Node ID')
    ax2.set_ylabel('Projected Load')
    ax2.set_title('Projected Load After Adding New Task')
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels([f'cpu{i}' for i in df_detail['node_id']])
    ax2.legend()
    
    # Plot 3: Performance factor vs load relationship
    ax3.scatter(df_detail['perf_factor'], df_detail['greedy_current'], 
               s=100, alpha=0.7, label='Greedy', color='blue')
    ax3.scatter(df_detail['perf_factor'], df_detail['knap_current'], 
               s=100, alpha=0.7, label='Knapsack', color='green', marker='s')
    
    for i, (x, y1, y2) in enumerate(zip(df_detail['perf_factor'], 
                                       df_detail['greedy_current'], 
                                       df_detail['knap_current'])):
        ax3.annotate(f'cpu{i}', (x, y1), xytext=(5, 5), 
                    textcoords='offset points', fontsize=8, color='blue')
        ax3.annotate(f'cpu{i}', (x, y2), xytext=(5, -15), 
                    textcoords='offset points', fontsize=8, color='green')
    
    ax3.set_xlabel('Performance Factor')
    ax3.set_ylabel('Current Load')
    ax3.set_title('Performance vs Load Trade-off')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    # Plot 4: Ranking comparison
    strategies = ['Greedy', 'Knapsack']
    ranks = [df_detail['greedy_rank'].values, df_detail['knap_rank'].values]
    colors = ['lightblue', 'lightgreen']
    
    for i, (strategy, rank_data, color) in enumerate(zip(strategies, ranks, colors)):
        ax4.bar(x_pos + i*width, rank_data, width, label=strategy, 
               color=color, alpha=0.8)
    
    ax4.set_xlabel('Node ID')
    ax4.set_ylabel('Rank (1=Best Choice)')
    ax4.set_title('Node Ranking by Strategy')
    ax4.set_xticks(x_pos + width/2)
    ax4.set_xticklabels([f'cpu{i}' for i in df_detail['node_id']])
    ax4.legend()
    ax4.set_ylim(0, 7)
    
    plt.tight_layout()
    plt.savefig('detailed_scenario_analysis.png', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    # Load comprehensive data
    print("Loading comprehensive analysis data...")
    df_comprehensive = load_and_analyze_comprehensive_data()
    
    # Create visualizations
    print("Creating strategy comparison heatmap...")
    plot_strategy_comparison_heatmap(df_comprehensive)
    
    print("Creating performance analysis plots...")
    plot_performance_differences(df_comprehensive)
    
    # Load and plot detailed scenario if available
    try:
        df_detail = pd.read_csv('detailed_scenario_analysis.csv')
        print("Creating detailed scenario analysis...")
        plot_detailed_scenario(df_detail)
    except FileNotFoundError:
        print("Detailed scenario file not found. Run C++ code first.")
    
    # Print summary statistics
    print("\n=== COMPREHENSIVE ANALYSIS SUMMARY ===")
    print(f"Total scenarios tested: {len(df_comprehensive)}")
    
    overall_winner = df_comprehensive['Winner'].value_counts()
    print(f"\nOverall strategy performance:")
    for strategy, count in overall_winner.items():
        percentage = (count / len(df_comprehensive)) * 100
        print(f"  {strategy}: {count} wins ({percentage:.1f}%)")
    
    print(f"\nAverage performance difference: {df_comprehensive['Time_Diff'].mean():.2f}")
    print(f"Maximum performance difference: {df_comprehensive['Time_Diff'].max():.2f}")
    
    # Performance by heterogeneity level
    print(f"\nPerformance by heterogeneity level:")
    hetero_summary = df_comprehensive.groupby('Perf_Name')['Winner'].apply(
        lambda x: f"Knapsack: {(x=='Knapsack').mean():.2f}, Greedy: {(x=='Greedy').mean():.2f}"
    )
    for perf_type, summary in hetero_summary.items():
        print(f"  {perf_type}: {summary}")

if __name__ == "__main__":
    main()