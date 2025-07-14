// A DP based CPP program for painter's partition problem 
#include <climits> 
#include <iostream> 
#include <vector>

#include "metric_utils.h"

using namespace std; 

// function to calculate sum between two indices 
// in array 
long  sum(vector<long> arr, int from, int to);

// bottom up tabular dp 
long  findMax(vector<long> arr, int n, int k); 
bool isPartitionPossible(vector<long> arr, int n, int k, long maxTime);
long minWeight(vector<long> wgts, int n, int k);
// driver function 
vector<int>  painterPartition(const amrex::BoxArray&   boxes,vector<long> arr,int k, int r = 0);
vector<vector<int>> painterPartition_VecVec(const amrex::BoxArray &boxes, vector<long> wgts, int number_of_ranks);
std::vector<int>
SFCProcessorMapDoItCombinedPainter (const amrex::BoxArray&          boxes,
                     const std::vector<amrex::Long>& wgts,
                     int                             nnodes,
                     int                             ranks_per_node,
                     amrex::Real*                    sfc_eff,
                     amrex::Real*                    knapsack_eff,
                     bool                            flag_verbose_mapper,
                     bool                            sort,
                     int                             r, // run number
                     const std::vector<amrex::Long>& bytes); 