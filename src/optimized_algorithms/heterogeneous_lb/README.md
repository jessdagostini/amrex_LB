# Heterogeneous Load Balancer (Proof of Concept)

This project demonstrates a proof-of-concept heterogeneous load balancer for assigning tasks to nodes with different performance characteristics (e.g., CPUs/GPUs with different speeds).

## Key Features
- Assigns new tasks to the node that will minimize projected completion time, based on current load and node performance.
- Simulates a knapsack-style load balancing approach, but uses timing (performance) instead of just load.
- Visualizes projected times for each node using a Python bar graph.

## Structure
- `heterogeneous_lb.cpp` / `.h`: C++ implementation of the core logic.
- `test_heterogeneous_lb.cpp`: Simple test harness for the load balancer.
- `plot_heterogeneous_lb.py`: Python script to visualize the results.

## Usage
1. Build the C++ code (see Makefile or build instructions).
2. Run the test harness to generate output data.
3. Use the Python script to plot the projected times and node assignments.

## Requirements
- C++17 or later
- Python 3.x with matplotlib

## Author
- Proof-of-concept for heterogeneous load balancing, inspired by AMReX and research on load balancing for heterogeneous systems. 