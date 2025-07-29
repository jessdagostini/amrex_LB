#include "metric_utils.h"

entryPoint *metric_data = NULL;

long long int entryPointCount = 0;
pthread_rwlock_t lock_metric_utils;

char algorithms[7][40] = {"Knapsack", "Karmarkar-Karp", "SFC", "SFC+Painter", "SFC+Knapsack","Painter+Knapsack", "Original"};
char metrics[7][40] = {"Efficiency", "Time", "Memory", "Weight", "PreviousEfficiency", "UsedEfficiency"};

void metric_utils_dump() {
    // fprintf(stderr, "Entrou dump\n");
    entryPoint *s = NULL;
    entryPoint *tmp = NULL;

    // fprintf(stderr, "\nTime Measurements\n");
    HASH_ITER(hh, metric_data, s, tmp) {
        fprintf(stderr, "%d, %s, %s, %0.10f, %d\n", 
                s->run_id,
                algorithms[s->id_algorithm],
                metrics[s->id_metric],
                s->metric_value,
                s->rank_id);
    }
}

void metric_utils_add(int id_algorithm, int id_metric, double metric_value, int run_id, int rank_id) {
    if (pthread_rwlock_wrlock(&lock_metric_utils) != 0) {
        fprintf(stderr, "Can't get mutex\n");
        exit(-1);
    }

    entryPoint *s = (entryPoint *) malloc(sizeof *s);

    s->id_algorithm = id_algorithm;
    s->id_metric = id_metric;
    s->metric_value = metric_value;
    s->run_id = run_id; // Store the run ID
    s->rank_id = rank_id;
    s->id = entryPointCount;

    HASH_ADD_INT(metric_data, id, s);  /* id is the key field */

    entryPointCount++;

    pthread_rwlock_unlock(&lock_metric_utils);
    // pthread_mutex_unlock(&incoming_queue_mutex);
}