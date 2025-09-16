#include <AMReX_Extension.H>
#include <AMReX_IntVect.H>
#include <AMReX_Morton.H>
#include <AMReX_Print.H>
#include <AMReX_ParallelContext.H>

// Add Hilbert includes
#include "HilbertSFC.H"
#include "HilbertSFCToken.H"

// A DP based CPP program for painter's partition problem 
#include <climits> 
#include <iostream> 
#include <vector>
#include <numeric>  // for std::iota
#include <SFC.H>
#include <Knapsack.H>
#include <LeastUsed.H>
#include "painterPartition.H"
using namespace std; 

// Enum for space-filling curve type
// enum class SFCType { MORTON, HILBERT };

// function to calculate sum between two indices in wgts
long sum(vector<long> wgts, int from, int to) { 
    long int total = 0; 
    for (int i = from; i <= to; i++) 
        total += wgts[i]; 
    
    return total; 
} 

// bottom up tabular dp 
long findMax(vector<long> wgts, int n, int k) { 
    // initialize table 
    long int dp[k + 1][n + 1] = { 0 }; 

    // base cases 
    // k=1 
    for (int i = 1; i <= n; i++) 
        dp[1][i] = sum(wgts, 0, i - 1); 

    // n=1 
    for (int i = 1; i <= k; i++) 
        dp[i][1] = wgts[0]; 

    // 2 to k partitions 
    for (int i = 2; i <= k; i++) { // 2 to n boards 
        for (int j = 2; j <= n; j++) { 

            // track minimum 
            long best = LONG_MAX; 
            
            // i-1 th separator before position wgts[p=1..j] 
            for (int p = 1; p <= j; p++) 
                best = min(best, max(dp[i - 1][p], 
                            sum(wgts, p, j - 1)));     

            dp[i][j] = best; 
        } 
    } 
    
    return dp[k][n]; 
} 

bool isPartitionPossible(vector<long> wgts, int n, int number_of_ranks, long maxWeight) {
    long long currWeight=0, worker=1;
    for(int i=0;i<n;i++)
    {
        if(currWeight+wgts[i]>maxWeight){
            worker++;
            
            if(worker>number_of_ranks) return false;
            currWeight=wgts[i];
        }
        else currWeight+=wgts[i];
    }
    return true;   
}

long minWeight(vector<long> wgts, int n, int k) {
    // code here
    // return minimum time
    long long sum=0,max=wgts[0];
    // if(k>n) return -1;
    for(int i=0;i<n;i++){
        sum+=wgts[i];
        if(max<wgts[i]){
            max=wgts[i];
        }
    }

    long long avg = sum / k;
    long long l = (max > avg) ? max : avg;  // max(largest_task, average_load)
    long long h = avg + max;                // average_load + largest_task
    
    long long mid, res = h;
    int numIterations = 0;
    while(l<=h){
        mid=l+(h-l)/2;
        if(isPartitionPossible(wgts,n,k,mid)){
            res=mid;
            h=mid-1;
        }
        else{
            l=mid+1;
        }
        numIterations++;
    }
    amrex::Print() << "Binary search iterations: " << numIterations << '\n';
    return res;
}

// Generic painter partition function with SFC type selection
vector<int> painterPartition(const amrex::BoxArray& boxes, 
                           vector<long> wgts, 
                           int number_of_ranks,
                           amrex::Real &sfc_painter_eff,
                           bool full,
                           SFCType sfc_type) { 
    BL_PROFILE("painterPartition()");
    vector<long> sorted_wgts;
    
    const int N = boxes.size();
    vector<int> sorted_result(wgts.size());
    
    // Generate tokens based on SFC type
    if (sfc_type == SFCType::MORTON) {
        std::vector<SFCToken> tokens;
        tokens.reserve(N);
        for (int i = 0; i < N; ++i) {
            const amrex::Box& bx = boxes[i];
            tokens.push_back(makeSFCToken(i, bx.smallEnd()));
        }
        
        std::sort(tokens.begin(), tokens.end(), SFCToken::Compare());
        for (int i = 0; i < N; ++i) {
            sorted_wgts.push_back(wgts[tokens[i].m_box]);
        }
        
        // Apply painter's partitioning
        long int n = wgts.size();
        long maxVal = minWeight(sorted_wgts, n, number_of_ranks);
        vector<vector<int>> vec(number_of_ranks);
        
        // Partition logic
        for(int i=0, j=i, p=0; p<number_of_ranks && j<n; p++){
            long val = sum(sorted_wgts, i, j);
            while(maxVal > val && j < n){
                j++;
                val = sum(sorted_wgts, i, j);
            }
            
            if(maxVal == val){
                for(int a=i; a<=j; a++){
                    vec[p].push_back(a);
                    sorted_result[a] = p;
                }
                j++;
                i = j;
            }
            else{
                for(int a=i; a<j; a++){
                    vec[p].push_back(a);
                    sorted_result[a] = p;
                }
                i = j;
            }
        }
        
        // Map back to original box indices
        vector<int> result(wgts.size());
        for (int i = 0; i < N; ++i) {
            result[tokens[i].m_box] = sorted_result[i];
        }
        
        // Calculate and print efficiency
        std::vector<LIpair> LIpairV;
        LIpairV.reserve(number_of_ranks);
        
        for (int i = 0; i < number_of_ranks; ++i) {
            amrex::Long wgt = 0;
            const std::vector<int>& vi = vec[i];
            for (int j = 0, M = vi.size(); j < M; ++j) {
                wgt += sorted_wgts[vi[j]];
            }
            LIpairV.push_back(LIpair(wgt, i));
        }
        
        Sort(LIpairV, true);

        if (full){
            for (const auto &p : LIpairV) {
                // metric_utils_add(MetricUtilsAlgorithms::SFC_PAINTER, MetricUtilsMetrics::WEIGHT, p.first, r, p.second);
            }
        }

        amrex::Real sum_wgt = 0, max_wgt = 0;
        for (int i = 0; i < number_of_ranks; ++i) {
            const amrex::Long W = LIpairV[i].first;
            if (W > max_wgt) max_wgt = W;
            sum_wgt += W;
        }
        amrex::Real efficiency = (sum_wgt/(number_of_ranks*max_wgt));
        sfc_painter_eff = efficiency;
        
        if (full) {
            amrex::Print() << "SFC+painterPartition efficiency: " << efficiency << '\n';
            // metric_utils_add(MetricUtilsAlgorithms::SFC_PAINTER, MetricUtilsMetrics::EFFICIENCY, efficiency, r);
        }
        
        return result;
    }
    else if (sfc_type == SFCType::HILBERT) {
        std::vector<HilbertSFCToken> tokens;
        tokens.reserve(N);
        for (int i = 0; i < N; ++i) {
            const amrex::Box& bx = boxes[i];
            tokens.push_back(makeHilbertSFCToken(i, bx.smallEnd()));
        }
        
        std::sort(tokens.begin(), tokens.end(), HilbertSFCToken::Compare());
        for (int i = 0; i < N; ++i) {
            sorted_wgts.push_back(wgts[tokens[i].m_box]);
        }
        
        // Apply painter's partitioning (same logic as Morton)
        long int n = wgts.size();
        long maxVal = minWeight(sorted_wgts, n, number_of_ranks);
        vector<vector<int>> vec(number_of_ranks);
        
        for(int i=0, j=i, p=0; p<number_of_ranks && j<n; p++){
            long val = sum(sorted_wgts, i, j);
            while(maxVal > val && j < n){
                j++;
                val = sum(sorted_wgts, i, j);
            }
            
            if(maxVal == val){
                for(int a=i; a<=j; a++){
                    vec[p].push_back(a);
                    sorted_result[a] = p;
                }
                j++;
                i = j;
            }
            else{
                for(int a=i; a<j; a++){
                    vec[p].push_back(a);
                    sorted_result[a] = p;
                }
                i = j;
            }
        }
        
        // Map back to original box indices
        vector<int> result(wgts.size());
        for (int i = 0; i < N; ++i) {
            result[tokens[i].m_box] = sorted_result[i];
        }
        
        // Calculate and print efficiency
        std::vector<LIpair> LIpairV;
        LIpairV.reserve(number_of_ranks);
        
        for (int i = 0; i < number_of_ranks; ++i) {
            amrex::Long wgt = 0;
            const std::vector<int>& vi = vec[i];
            for (int j = 0, M = vi.size(); j < M; ++j) {
                wgt += sorted_wgts[vi[j]];
            }
            LIpairV.push_back(LIpair(wgt, i));
        }
        
        Sort(LIpairV, true);

        amrex::Real sum_wgt = 0, max_wgt = 0;
        for (int i = 0; i < number_of_ranks; ++i) {
            const amrex::Long W = LIpairV[i].first;
            if (W > max_wgt) max_wgt = W;
            sum_wgt += W;
        }
        amrex::Real efficiency = (sum_wgt/(number_of_ranks*max_wgt));
        sfc_painter_eff = efficiency;
        amrex::Print() << "Hilbert+painterPartition efficiency: " << efficiency << '\n';
        
        return result;
    }
    
    // Fallback to Morton if unknown type
    return painterPartition(boxes, wgts, number_of_ranks, sfc_painter_eff, full, SFCType::MORTON);
}

// Wrapper functions for backward compatibility and easy access
vector<int> painterPartitionMorton(const amrex::BoxArray& boxes,
                                 vector<long> wgts,
                                 int number_of_ranks,
                                 bool full,
                                 amrex::Real &sfc_painter_eff) {
    return painterPartition(boxes, wgts, number_of_ranks, sfc_painter_eff, full, SFCType::MORTON);
}

vector<int> painterPartitionHilbert(const amrex::BoxArray& boxes,
                                  vector<long> wgts,
                                  int number_of_ranks,
                                  amrex::Real &sfc_painter_eff,
                                  bool full) {
    return painterPartition(boxes, wgts, number_of_ranks, sfc_painter_eff, full, SFCType::HILBERT);
}

// Updated VecVec version with SFC type selection
vector<vector<int>> painterPartition_VecVec(const amrex::BoxArray& boxes, 
                                           vector<long> wgts, 
                                           int number_of_ranks,
                                           SFCType sfc_type) { 
    BL_PROFILE("painterPartition_combined()");
    vector<long> sorted_wgts;
    
    const int N = boxes.size();
    
    // Generate tokens based on SFC type
    if (sfc_type == SFCType::MORTON) {
        std::vector<SFCToken> tokens;
        tokens.reserve(N);
        for (int i = 0; i < N; ++i) {
            const amrex::Box& bx = boxes[i];
            tokens.push_back(makeSFCToken(i, bx.smallEnd()));
        }
        
        std::sort(tokens.begin(), tokens.end(), SFCToken::Compare());
        for (int i = 0; i < N; ++i) {
            sorted_wgts.push_back(wgts[tokens[i].m_box]);
        }
    }
    else if (sfc_type == SFCType::HILBERT) {
        std::vector<HilbertSFCToken> tokens;
        tokens.reserve(N);
        for (int i = 0; i < N; ++i) {
            const amrex::Box& bx = boxes[i];
            tokens.push_back(makeHilbertSFCToken(i, bx.smallEnd()));
        }
        
        std::sort(tokens.begin(), tokens.end(), HilbertSFCToken::Compare());
        for (int i = 0; i < N; ++i) {
            sorted_wgts.push_back(wgts[tokens[i].m_box]);
        }
    }

    long int n = wgts.size();
    long maxVal = minWeight(sorted_wgts, n, number_of_ranks);
    vector<vector<int>> vec(number_of_ranks);
    vector<int> sorted_result(wgts.size());
    
    amrex::Real s_painter_eff = 0.0;
    bool sort = true;
    int nteams = number_of_ranks;
    bool flag_verbose_mapper = true;
    
    for(int i=0, j=i, p=0; p<number_of_ranks && j<n; p++){
        long val = sum(sorted_wgts, i, j);
        while(maxVal > val && j < n){
            j++;
            val = sum(sorted_wgts, i, j);
        }
        
        if(maxVal == val){
            for(int a=i; a<=j; a++){
                vec[p].push_back(a);
                sorted_result[a] = p;
            }
            j++;
            i = j;
        }
        else{
            for(int a=i; a<j; a++){
                vec[p].push_back(a);
                sorted_result[a] = p;
            }
            i = j;
        }
    }
    
    // Calculate and print efficiency
    std::vector<LIpair> LIpairV;
    LIpairV.reserve(nteams);
    
    for (int i = 0; i < nteams; ++i) {
        amrex::Long wgt = 0;
        const std::vector<int>& vi = vec[i];
        for (int j = 0, M = vi.size(); j < M; ++j) {
            wgt += sorted_wgts[vi[j]];
        }
        LIpairV.push_back(LIpair(wgt, i));
    }
    
    if (sort) Sort(LIpairV, true);

    if (s_painter_eff || flag_verbose_mapper) {
        amrex::Real sum_wgt = 0, max_wgt = 0;
        for (int i = 0; i < nteams; ++i) {
            const amrex::Long W = LIpairV[i].first;
            if (W > max_wgt) max_wgt = W;
            sum_wgt += W;
            // metric_utils_add(MetricUtilsAlgorithms::SFC_PAINTER, MetricUtilsMetrics::WEIGHT, W, r, i);
        }
        amrex::Real efficiency = (sum_wgt/(nteams*max_wgt));
        if (s_painter_eff) s_painter_eff = efficiency;
        // sfc_painter_eff = efficiency;

        if (flag_verbose_mapper) {
            const char* sfc_name = (sfc_type == SFCType::HILBERT) ? "Hilbert" : "SFC";
            amrex::Print() << sfc_name << "+painterPartition efficiency: " << efficiency << '\n';
        }
    }
     
    return vec;
}

// Wrapper functions for VecVec version
vector<vector<int>> painterPartition_VecVecMorton(const amrex::BoxArray& boxes, 
                                                 vector<long> wgts, 
                                                 int number_of_ranks) {
    return painterPartition_VecVec(boxes, wgts, number_of_ranks, SFCType::MORTON);
}

vector<vector<int>> painterPartition_VecVecHilbert(const amrex::BoxArray& boxes, 
                                                  vector<long> wgts, 
                                                  int number_of_ranks) {
    return painterPartition_VecVec(boxes, wgts, number_of_ranks, SFCType::HILBERT);
}