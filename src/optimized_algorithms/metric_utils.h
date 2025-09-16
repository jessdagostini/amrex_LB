#ifndef METRICUTILS_H
#define METRICUTILS_H

#include <iostream>

using namespace std;

extern char algorithms[10][40];
extern char metrics[8][40];

enum MetricUtilsAlgorithms {
    KNAPSACK,          // Automatically assigned 0
    KARMARKAR_KARP,       // Automatically assigned 1
    SFC,                 // Automatically assigned 2
    SFC_PAINTER,      // Automatically assigned 3
    SFC_KNAPSACK,        // Automatically assigned 4
    PAINTER_KNAPSACK, // Automatically assigned 5
    ORIGINAL,            // Automatically assigned 6
    HILBERT_SFC,        // Automatically assigned 7
    HILBERT_PAINTER,     // Automatically assigned 8
    HILBERT_KNAPSACK,    // Automatically assigned 9
};

enum MetricUtilsMetrics {
    EFFICIENCY,     // Automatically assigned 0
    TIME,           // Automatically assigned 1
    MEMORY,        // Automatically assigned 2
    WEIGHT,       // Automatically assigned 3
    PREVIOUS_EFFICIENCY, // Automatically assigned 4
    USED_EFFICIENCY // Automatically assigned 5
};
#endif