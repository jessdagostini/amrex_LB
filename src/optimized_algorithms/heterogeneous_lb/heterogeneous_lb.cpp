#include "heterogeneous_lb.h"
#include "../Knapsack.H"
#include <iostream>
#include <vector>
#include <random>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <AMReX_INT.H>
#include <numeric>
#include <fstream>

struct HeterogeneityResult {
    std::string test_name;
    double heterogeneity_factor;
    double homo_efficiency;
    double performance_aware_efficiency;
    double relation_aware_efficiency;
    double homo_makespan;
    double performance_aware_makespan;
    double relation_aware_makespan;
};

// Compute rij matrix: rij[i][j] = perf[j] / perf[i]
std::vector<std::vector<double>> compute_rij(const std::vector<Node>& nodes) {
    size_t n = nodes.size();
    std::vector<std::vector<double>> rij(n, std::vector<double>(n, 1.0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            rij[i][j] = nodes[j].performance_factor / nodes[i].performance_factor;
        }
    }
    return rij;
}

// Print rij matrix
void print_rij_matrix(const std::vector<std::vector<double>>& rij, const std::vector<Node>& nodes) {
    std::cout << "rij matrix (rij[i][j] = perf[j]/perf[i]):\n";
    std::cout << "     ";
    for (size_t j = 0; j < nodes.size(); ++j) {
        std::cout << std::setw(6) << "N" + std::to_string(j);
    }
    std::cout << "\n";
    
    for (size_t i = 0; i < rij.size(); ++i) {
        std::cout << "N" << i << "   ";
        for (size_t j = 0; j < rij[i].size(); ++j) {
            std::cout << std::setw(6) << std::setprecision(2) << std::fixed << rij[i][j];
        }
        std::cout << " (perf=" << nodes[i].performance_factor << ")\n";
    }
    std::cout << "\n";
}

// Homogeneous knapsack - all nodes have performance_factor = 1.0
std::vector<int> homogeneous_knapsack_assignment(const std::vector<Task>& tasks, int num_nodes) {
    std::vector<amrex::Long> weights;
    weights.reserve(tasks.size());
    for (const auto& task : tasks) {
        weights.push_back(static_cast<amrex::Long>(task.base_time * 1000));
    }
    amrex::Real efficiency = 0.0;
    return KnapSackDoIt(weights, num_nodes, efficiency, true,
                        std::numeric_limits<int>::max(), false, false,
                        std::vector<amrex::Long>());
}

// Heterogeneous performance aware knapsack - basic performance-aware approach
std::vector<int> performance_aware_knapsack_assignment(const std::vector<Task>& tasks, const std::vector<Node>& nodes) {
    std::vector<int> assignments(tasks.size());
    std::vector<double> node_loads(nodes.size(), 0.0);
    
    // Sort tasks by size (largest first) for better balancing
    std::vector<std::pair<double, int>> task_indices;
    for (size_t i = 0; i < tasks.size(); ++i) {
        task_indices.push_back({tasks[i].base_time, i});
    }
    std::sort(task_indices.rbegin(), task_indices.rend());
    
    // Assign each task using a naive performance-aware approach
    for (const auto& task_pair : task_indices) {
        int task_idx = task_pair.second;
        const Task& task = tasks[task_idx];
        
        // assign to the node with the lowest current load
        // scaled by performance factor (to account for heterogeneity)
        int best_node = 0;
        double min_scaled_load = std::numeric_limits<double>::max();
        
        for (size_t candidate = 0; candidate < nodes.size(); ++candidate) {
            // Scale current load by inverse of performance factor
            // This gives preference to faster nodes but doesn't account for task size
            double scaled_load = node_loads[candidate] + (task.base_time / nodes[candidate].performance_factor);
            
            if (scaled_load < min_scaled_load) {
                min_scaled_load = scaled_load;
                best_node = candidate;
            }
        }
        
        // Assign task and update load
        assignments[task_idx] = best_node;
        node_loads[best_node] += task.base_time / nodes[best_node].performance_factor;
    }
    
    return assignments;
}

// Heterogeneous relation-aware knapsack - rij matrix for better load balancing
std::vector<int> relation_aware_knapsack_assignment(const std::vector<Task>& tasks, const std::vector<Node>& nodes, const std::vector<std::vector<double>>& rij) {
    std::vector<int> assignments(tasks.size());
    std::vector<double> node_loads(nodes.size(), 0.0);

    // Calculate total work
    double total_work = 0.0;
    for (const auto& task : tasks) {
        total_work += task.base_time;
    }

    // Sort tasks by size (largest first)
    std::vector<std::pair<double, int>> task_indices;
    for (size_t i = 0; i < tasks.size(); ++i) {
        task_indices.push_back({tasks[i].base_time, i});
    }
    std::sort(task_indices.rbegin(), task_indices.rend());
    
    for (const auto& task_pair : task_indices) {
        int task_idx = task_pair.second;
        const Task& task = tasks[task_idx];
        
        int best_node = 0;
        double best_score = std::numeric_limits<double>::max();
        
        for (size_t candidate = 0; candidate < nodes.size(); ++candidate) {
            double exec_time = task.base_time / nodes[candidate].performance_factor;
            double completion_time = node_loads[candidate] + exec_time;
            
            // Use rij matrix for load balancing awareness
            double load_balance_score = completion_time;
            
            // Factor 1: Consider how this assignment affects overall balance
            // Look at load distribution after this assignment
            double projected_load = node_loads[candidate] + exec_time;
            double balance_penalty = 0.0;

            // Calculate load variance among nodes
            double mean_load = std::accumulate(node_loads.begin(), node_loads.end(), 0.0) / nodes.size();
            double load_variance = 0.0;
            for (double load : node_loads) {
                load_variance += (load - mean_load) * (load - mean_load);
            }
            load_variance /= nodes.size();

            for (size_t other = 0; other < nodes.size(); ++other) {
                if (other != candidate) {
                    // Use rij to understand relative capacity
                    double relative_capacity = rij[other][candidate]; // How much faster 'candidate' is vs 'other'
                    double other_relative_load = node_loads[other] * relative_capacity;

                    // Dynamic imbalance threshold based on current loads and variance
                    double imbalance_threshold = 1.0 + (node_loads[candidate] / (total_work / nodes.size())) + (load_variance * 0.1);
                    if (projected_load > other_relative_load * imbalance_threshold) {
                        balance_penalty += (projected_load - other_relative_load) * 0.3;
                    }
                }
            }

            // Factor 2: Prefer nodes that can handle load redistribution well
            // Nodes with good rij relationships to other nodes are preferred
            double redistribution_bonus = 0.0;
            for (size_t other = 0; other < nodes.size(); ++other) {
                if (other != candidate) {
                    double rij_val = rij[candidate][other];
                    // Bonus for nodes that have good load transfer potential
                    if (rij_val >= 0.8 && rij_val <= 1.25) { // Similar performance range
                        redistribution_bonus += 0.15 * exec_time;
                    }
                }
            }

            // Factor 3: Consider node capacity
            double node_capacity = nodes[candidate].performance_factor;
            double capacity_bonus = node_capacity * 0.1 * exec_time;

            load_balance_score = completion_time + balance_penalty - redistribution_bonus + capacity_bonus;

            if (load_balance_score < best_score) {
                best_score = load_balance_score;
                best_node = candidate;
            }
        }

        assignments[task_idx] = best_node;
        node_loads[best_node] += task.base_time / nodes[best_node].performance_factor;
    }

    return assignments;
}

std::vector<HeterogeneityResult> test_heterogeneity_levels(const std::vector<Task>& tasks) {
    std::vector<HeterogeneityResult> results;

    // Real AMReX benchmark timings for each test (in seconds)
    std::vector<std::string> test_names = {
        "mb", "cb", "br", "dbl sum", "max", "scn", "jac", "jsy", "jex", "jmp", "aos smp", "aos sha", "gsrb", "parser"
    };

    std::vector<double> cpu_times = {1.02e-02, 2.05e-01, 2.20e-02, 1.44e-02, 6.63e-03, 9.75e-03, 2.74e-02, 1.79e-02, 1.78e-02, 1.75e-02, 1.96e-01, 1.96e-01, 4.60e+00, 6.50e-01};
    std::vector<double> gb40_times = {3.10e-04, 8.74e-04, 2.15e-04, 1.59e-04, 1.70e-04, 2.63e-04, 6.44e-04, 7.38e-04, 6.68e-04, 6.48e-04, 3.44e-02, 2.37e-03, 4.85e-04, 1.69e-03};
    std::vector<double> gb80_times = {2.46e-04, 8.01e-04, 1.73e-04, 1.42e-04, 1.43e-04, 2.21e-04, 5.38e-04, 6.31e-04, 5.53e-04, 5.42e-04, 3.45e-02, 1.93e-03, 4.25e-04, 1.51e-03};

    // For each test, create a config with 2 cpu, 3 40gb, 3 80gb nodes
    for (size_t test_idx = 0; test_idx < test_names.size(); ++test_idx) {
        double cpu_time = cpu_times[test_idx];
        double gb40_time = gb40_times[test_idx];
        double gb80_time = gb80_times[test_idx];

        std::vector<double> perf_factors = {
            cpu_time / cpu_time, cpu_time / cpu_time,
            cpu_time / gb40_time, cpu_time / gb40_time, cpu_time / gb40_time,
            cpu_time / gb80_time, cpu_time / gb80_time, cpu_time / gb80_time
        };

        std::vector<Node> nodes;
        for (size_t i = 0; i < perf_factors.size(); ++i) {
            nodes.push_back(Node(i, "cpu" + std::to_string(i), perf_factors[i], 0.0));
        }

        // Homogeneous nodes (all perf = 1.0)
        std::vector<Node> homo_nodes;
        for (size_t i = 0; i < perf_factors.size(); ++i) {
            homo_nodes.push_back(Node(i, "cpu" + std::to_string(i), 1.0, 0.0));
        }

        // Compute rij matrix
        auto rij = compute_rij(nodes);

        // Run all three algorithms
        auto homo_assign = homogeneous_knapsack_assignment(tasks, homo_nodes.size());
        auto performance_aware_assign = performance_aware_knapsack_assignment(tasks, nodes);
        auto relation_aware_assign = relation_aware_knapsack_assignment(tasks, nodes, rij);

        // Calculate loads and metrics
        std::vector<double> homo_loads(nodes.size(), 0.0);
        std::vector<double> performance_aware_loads(nodes.size(), 0.0);
        std::vector<double> relation_aware_loads(nodes.size(), 0.0);

        double total_work = 0.0;
        for (const auto& task : tasks) {
            total_work += task.base_time;
        }

        for (size_t i = 0; i < tasks.size(); ++i) {
            homo_loads[homo_assign[i]] += tasks[i].base_time / 1.0;
            performance_aware_loads[performance_aware_assign[i]] += tasks[i].base_time / nodes[performance_aware_assign[i]].performance_factor;
            relation_aware_loads[relation_aware_assign[i]] += tasks[i].base_time / nodes[relation_aware_assign[i]].performance_factor;
        }

        double homo_makespan = *std::max_element(homo_loads.begin(), homo_loads.end());
        double performance_aware_makespan = *std::max_element(performance_aware_loads.begin(), performance_aware_loads.end());
        double relation_aware_makespan = *std::max_element(relation_aware_loads.begin(), relation_aware_loads.end());

        double total_capacity = std::accumulate(perf_factors.begin(), perf_factors.end(), 0.0);
        double ideal_makespan = total_work / total_capacity;

        HeterogeneityResult result;
        result.test_name = test_names[test_idx];
        result.heterogeneity_factor = *std::max_element(perf_factors.begin(), perf_factors.end()) / *std::min_element(perf_factors.begin(), perf_factors.end());
        result.homo_efficiency = ideal_makespan / homo_makespan;
        result.performance_aware_efficiency = ideal_makespan / performance_aware_makespan;
        result.relation_aware_efficiency = ideal_makespan / relation_aware_makespan;
        result.homo_makespan = homo_makespan;
        result.performance_aware_makespan = performance_aware_makespan;
        result.relation_aware_makespan = relation_aware_makespan;

        results.push_back(result);

        // Print logistics for this test
        std::cout << "Test: " << test_names[test_idx] << "\n";
        std::cout << "  CPU time: " << cpu_time << ", 40GB time: " << gb40_time << ", 80GB time: " << gb80_time << "\n";
        std::cout << "  Performance factors: ";
        for (auto pf : perf_factors) std::cout << std::setprecision(2) << std::fixed << pf << " ";
        std::cout << "\n";
        std::cout << "  Heterogeneity factor: " << std::setprecision(2) << std::fixed << result.heterogeneity_factor << "\n";
        std::cout << "  Ideal makespan: " << std::setprecision(2) << std::fixed << ideal_makespan << "\n";
        std::cout << "  Homogeneous makespan: " << std::setprecision(2) << std::fixed << homo_makespan << ", efficiency: " << std::setprecision(3) << result.homo_efficiency << "\n";
        std::cout << "  Performance Aware makespan: " << std::setprecision(2) << std::fixed << performance_aware_makespan << ", efficiency: " << std::setprecision(3) << result.performance_aware_efficiency << "\n";
        std::cout << "  Relation Aware makespan: " << std::setprecision(2) << std::fixed << relation_aware_makespan << ", efficiency: " << std::setprecision(3) << result.relation_aware_efficiency << "\n";
        std::cout << "-------------------------------------------------------------\n";
    }

    return results;
}

int main() {
    std::cout << "=== HETEROGENEOUS LOAD BALANCE SYSTEM TEST ===\n\n";

    double jac_cpu = 2.74e-02;
    double jac_40gb = 6.44e-04;
    double jac_80gb = 5.38e-04;

    // double pf_cpu = jac_40gb / jac_cpu;
    // double pf_40gb = jac_40gb / jac_40gb;
    // double pf_80gb = jac_40gb / jac_80gb;

    double pf_cpu = jac_cpu / jac_cpu; // baseline
    double pf_40gb = jac_cpu / jac_40gb;
    double pf_80gb = jac_cpu / jac_80gb;


    std::vector<double> perf_factors = {pf_cpu, pf_cpu, pf_40gb, pf_40gb, pf_40gb, pf_80gb, pf_80gb, pf_80gb};
    std::vector<Node> nodes;
    
    for (size_t i = 0; i < perf_factors.size(); ++i) {
        nodes.push_back(Node(i, "cpu" + std::to_string(i), perf_factors[i], 0.0));
    }
    
    // Compute and print rij matrix
    auto rij = compute_rij(nodes);
    print_rij_matrix(rij, nodes);
    
    // Generate tasks with higher variance
    std::vector<Task> tasks;
    std::mt19937 gen(42);
    std::uniform_real_distribution<> d(5.0, 100.0); // Wider range of task sizes
    double total_work = 0.0;

    std::cout << "Generating tasks...\n";
    for (int i = 0; i < 1000000; ++i) {
        double task_time = d(gen);
        tasks.push_back(Task(i, task_time));
        total_work += task_time;
    }
    
    std::cout << "Generated " << tasks.size() << " tasks, total work: " << total_work << "\n\n";
    
    // Homogeneous nodes: all perf = 1.0
    std::vector<Node> homo_nodes;
    for (size_t i = 0; i < nodes.size(); ++i) {
        homo_nodes.push_back(Node(i, "cpu" + std::to_string(i), 1.0, 0.0));
    }
    auto performance_aware_nodes = nodes;
    auto relation_aware_nodes = nodes;
    
    // Assignment
    auto homo_assign = homogeneous_knapsack_assignment(tasks, homo_nodes.size());
    auto performance_aware_assign = performance_aware_knapsack_assignment(tasks, performance_aware_nodes);
    auto relation_aware_assign = relation_aware_knapsack_assignment(tasks, relation_aware_nodes, rij);
    
    // Calculate loads for each approach
    std::vector<double> homo_loads(homo_nodes.size(), 0.0);
    std::vector<double> performance_aware_loads(nodes.size(), 0.0);
    std::vector<double> relation_aware_loads(nodes.size(), 0.0);
    
    // For homogeneous, use perf=1.0 for all nodes; for others, use actual perf factors
    for (size_t i = 0; i < tasks.size(); ++i) {
        // Homogeneous: calculate load as if all nodes have performance 1.0
        homo_loads[homo_assign[i]] += tasks[i].base_time / 1.0;
        // Performance Aware, Relation Aware: use actual performance factors
        performance_aware_loads[performance_aware_assign[i]] += tasks[i].base_time / nodes[performance_aware_assign[i]].performance_factor;
        relation_aware_loads[relation_aware_assign[i]] += tasks[i].base_time / nodes[relation_aware_assign[i]].performance_factor;
    }
    
    // Calculate metrics
    double homo_makespan = *std::max_element(homo_loads.begin(), homo_loads.end());
    double performance_aware_makespan = *std::max_element(performance_aware_loads.begin(), performance_aware_loads.end());
    double relation_aware_makespan = *std::max_element(relation_aware_loads.begin(), relation_aware_loads.end());
    
    double total_capacity = std::accumulate(perf_factors.begin(), perf_factors.end(), 0.0);
    double ideal_makespan = total_work / total_capacity;
    
    double homo_efficiency = ideal_makespan / homo_makespan;
    double performance_aware_efficiency = ideal_makespan / performance_aware_makespan;
    double relation_aware_efficiency = ideal_makespan / relation_aware_makespan;

    // // Debug: Print first 10 assignments
    // std::cout << "First 10 task assignments:\n";
    // std::cout << "Task | Homo | Performance Aware | Relation Aware\n";
    // for (int i = 0; i < std::min(10, (int)tasks.size()); ++i) {
    //     std::cout << std::setw(4) << i << " | " 
    //             << std::setw(4) << homo_assign[i] << " | "
    //             << std::setw(7) << performance_aware_assign[i] << " | "
    //             << std::setw(4) << relation_aware_assign[i] << "\n";
    // }
    
    // Print comparison
    std::cout << "=== RESULTS COMPARISON ===\n";
    std::cout << "Ideal makespan (perfect balance): " << std::setprecision(2) << std::fixed << ideal_makespan << "\n\n";

    std::cout << "       Approach         | Makespan  | Efficiency (Ideal/Makespan) | % from Ideal\n";
    std::cout << "------------------------|-----------|-----------------------------|-------------\n";
    std::cout << "1. Homogeneous Knapsack | " << std::setw(9) << std::setprecision(2) << std::fixed << homo_makespan
              << " | " << std::setw(27) << std::setprecision(3) << (ideal_makespan / homo_makespan)
              << " | " << std::setw(9) << std::setprecision(1) << ((homo_makespan - ideal_makespan) / ideal_makespan * 100) << "%\n";
    std::cout << "2. Performance Aware    | " << std::setw(9) << std::setprecision(2) << std::fixed << performance_aware_makespan
              << " | " << std::setw(27) << std::setprecision(3) << (ideal_makespan / performance_aware_makespan)
              << " | " << std::setw(9) << std::setprecision(1) << ((performance_aware_makespan - ideal_makespan) / ideal_makespan * 100) << "%\n";
    std::cout << "3. Relation Aware       | " << std::setw(9) << std::setprecision(2) << std::fixed << relation_aware_makespan
              << " | " << std::setw(27) << std::setprecision(3) << (ideal_makespan / relation_aware_makespan)
              << " | " << std::setw(9) << std::setprecision(1) << ((relation_aware_makespan - ideal_makespan) / ideal_makespan * 100) << "%\n\n";

    // std::cout << "Rij advantage over basic heterogeneous: " 
    //           << std::setprecision(1) << ((relation_aware_efficiency - performance_aware_efficiency) / performance_aware_efficiency * 100) << "%\n\n";
    
    // Show load distribution
    std::cout << "Load Distribution:\n";
    std::cout << "Node | Perf | Homogeneous | Performance Aware |  Relation Aware   \n";
    std::cout << "-----|------|-------------|-------------------|-------------------\n";

    for (size_t i = 0; i < nodes.size(); ++i) {
        std::cout << std::setw(4) << i << " | " 
                  << std::setw(4) << std::setprecision(1) << perf_factors[i] << " | "
                  << std::setw(11) << std::setprecision(2) << homo_loads[i] << " | "
                  << std::setw(17) << performance_aware_loads[i] << " | "
                  << std::setw(17) << relation_aware_loads[i] << "\n";
    }

    // Calculate actual work distribution (before performance scaling)
    std::vector<double> homo_work(homo_nodes.size(), 0.0);
    std::vector<double> performance_aware_work(nodes.size(), 0.0);
    std::vector<double> relation_aware_work(nodes.size(), 0.0);

    for (size_t i = 0; i < tasks.size(); ++i) {
        homo_work[homo_assign[i]] += tasks[i].base_time;
        performance_aware_work[performance_aware_assign[i]] += tasks[i].base_time;
        relation_aware_work[relation_aware_assign[i]] += tasks[i].base_time;
    }

    // Show work distribution vs execution time
    std::cout << "\nWork Distribution vs Execution Time:\n";
    std::cout << "Node | Perf | PA Work Amount | PA Execution Time | RA Work Amount | RA Execution Time\n";
    std::cout << "-----|------|----------------|-------------------|----------------|------------------\n";

    for (size_t i = 0; i < nodes.size(); ++i) {
        std::cout << std::setw(4) << i << " | " 
                << std::setw(4) << std::setprecision(1) << perf_factors[i] << " | "
                << std::setw(14) << std::setprecision(0) << performance_aware_work[i] << " | "
                << std::setw(17) << std::setprecision(2) << performance_aware_loads[i] << " | "
                << std::setw(14) << std::setprecision(0) << relation_aware_work[i] << " | "
                << std::setw(16) << std::setprecision(2) << relation_aware_loads[i] << "\n";
    }
    
    // Generate CSV data for plotting
    std::system("mkdir -p plots");
    
    // CSV for load distribution comparison
    std::ofstream csv_file("plots/load_distribution.csv");
    csv_file << "Node,Performance,Homogeneous,Performance_Aware,Relation_Aware\n";
    for (size_t i = 0; i < nodes.size(); ++i) {
        csv_file << i << "," << perf_factors[i] << "," 
                 << homo_loads[i] << "," << performance_aware_loads[i] << "," << relation_aware_loads[i] << "\n";
    }
    csv_file.close();
    
    // CSV for efficiency comparison
    std::ofstream eff_file("plots/efficiency_comparison.csv");
    eff_file << "Approach,Makespan,Efficiency\n";
    eff_file << "Homogeneous," << homo_makespan << "," << homo_efficiency << "\n";
    eff_file << "Performance_Aware," << performance_aware_makespan << "," << performance_aware_efficiency << "\n";
    eff_file << "Relation_Aware," << relation_aware_makespan << "," << relation_aware_efficiency << "\n";
    eff_file.close();

    std::cout << "\nLoad Distribution and Efficiency Comparison save to plots/load_distribution.csv, plots/efficiency_comparison.csv\n";

    std::cout << "\n=== HETEROGENEITY LEVEL ANALYSIS ===\n";
    auto het_results = test_heterogeneity_levels(tasks);

    // Save heterogeneity results to CSV
    std::ofstream het_file("plots/heterogeneity_analysis.csv");
    het_file << "Test_Name,Heterogeneity_Factor,Homo_Efficiency,Performance_Aware_Efficiency,Relation_Aware_Efficiency,";
    het_file << "Homo_Makespan,Performance_Aware_Makespan,Relation_Aware_Makespan\n";

    for (const auto& result : het_results) {
        het_file << result.test_name << ","
                 << result.heterogeneity_factor << ","
                 << result.homo_efficiency << ","
                 << result.performance_aware_efficiency << ","
                 << result.relation_aware_efficiency << ","
                 << result.homo_makespan << ","
                 << result.performance_aware_makespan << ","
                 << result.relation_aware_makespan << "\n";
    }
    het_file.close();

    std::cout << "Heterogeneity analysis saved to plots/heterogeneity_analysis.csv\n";

        return 0;
}

