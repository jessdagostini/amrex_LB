// A DP based CPP program for painter's partition problem 
#include <climits> 
#include <iostream> 
#include <vector>

#include "metric_utils.h"

using namespace std; 

// Enum for space-filling curve type
enum class SFCType { MORTON, HILBERT };

// function to calculate sum between two indices 
// in array 
long  sum(vector<long> arr, int from, int to);

// bottom up tabular dp 
long  findMax(vector<long> arr, int n, int k); 
bool isPartitionPossible(vector<long> arr, int n, int k, long maxTime);
long minWeight(vector<long> wgts, int n, int k);

// Main painterPartition function with SFC type parameter
vector<int> painterPartition(const amrex::BoxArray& boxes, vector<long> wgts, int number_of_ranks, amrex::Real &sfc_painter_eff, bool full = true, SFCType sfc_type = SFCType::MORTON);

// Wrapper functions for specific SFC types
vector<int> painterPartitionMorton(const amrex::BoxArray& boxes, vector<long> wgts, int number_of_ranks, amrex::Real &sfc_painter_eff, bool full = true);
vector<int> painterPartitionHilbert(const amrex::BoxArray& boxes, vector<long> wgts, int number_of_ranks, amrex::Real &sfc_painter_eff, bool full = true);

// VecVec versions
vector<vector<int>> painterPartition_VecVec(const amrex::BoxArray& boxes, vector<long> wgts, int number_of_ranks, SFCType sfc_type = SFCType::MORTON);
vector<vector<int>> painterPartition_VecVecMorton(const amrex::BoxArray& boxes, vector<long> wgts, int number_of_ranks);
vector<vector<int>> painterPartition_VecVecHilbert(const amrex::BoxArray& boxes, vector<long> wgts, int number_of_ranks);

std::vector<int>
SFCProcessorMapDoItCombinedPainter (const amrex::BoxArray&          boxes,
                     const std::vector<amrex::Long>& wgts,
                     int                             nnodes,
                     int                             ranks_per_node,
                     amrex::Real                    &sfc_eff,
                     amrex::Real                    &knapsack_eff,
                     bool                            flag_verbose_mapper,
                     bool                            sort,
                     const std::vector<amrex::Long>& bytes); 