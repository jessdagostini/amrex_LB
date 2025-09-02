#ifndef HETEROGENEOUS_LB_H
#define HETEROGENEOUS_LB_H

#include <vector>
#include <string>
#include <AMReX_INT.H>

struct Node {
    int id;
    std::string name;
    double performance_factor;
    double current_load;
    
    Node(int _id, const std::string& _name, double _perf, double _load = 0.0)
        : id(_id), name(_name), performance_factor(_perf), current_load(_load) {}
};

struct Task {
    int id;
    double base_time;
    
    Task(int _id, double _time) : id(_id), base_time(_time) {}
};

// Core functions
std::vector<std::vector<double>> compute_rij(const std::vector<Node>& nodes);

// Three main assignment approaches
std::vector<int> homogeneous_knapsack_assignment(const std::vector<Task>& tasks, const std::vector<Node>& nodes);
std::vector<int> performance_aware_knapsack_assignment(const std::vector<Task>& tasks, const std::vector<Node>& nodes);
std::vector<int> relation_aware_knapsack_assignment(const std::vector<Task>& tasks, const std::vector<Node>& nodes, const std::vector<std::vector<double>>& rij);

#endif