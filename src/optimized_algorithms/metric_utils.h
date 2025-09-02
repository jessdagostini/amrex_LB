#ifndef METRICUTILS_H
#define METRICUTILS_H

#include <iostream>
#include <omp.h>
#include <pthread.h>

#include "uthash.h"

using namespace std;

struct entryPoint {
    long long int id;
    int id_algorithm;
    int id_metric;
    double metric_value;
    int run_id; // Run ID to differentiate runs
    int rank_id;
    UT_hash_handle hh;
};

extern entryPoint *metric_data;

extern long long int entryPointCount;

extern pthread_rwlock_t lock_metric_utils;

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

void metric_utils_dump();

void metric_utils_add(int id_algorithm, int id_metric, double metric_value, int run_id, int rank_id = 0);
#endif