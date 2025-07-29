// AMReX dependencies
#include <AMReX.H>
#include <AMReX_Random.H>
#include <AMReX_MultiFab.H>
#include <AMReX_ParmParse.H>
#include <AMReX_ParallelDescriptor.H>

// C++ standard libraries
#include <random>
#include <string>
#include <cstring>
#include <time.h>  
#include <omp.h>
#include <fstream>
#include <numeric>
#include <cassert>

// LoadBalancing libraries
#include "Util.H"
#include "Knapsack.H"
#include "SFC.H"
#include "SFC_knapsack.H"
#include "painterPartition.H"
#include "KK.h"
#include "metric_utils.h"

#if defined(AMREX_USE_MPI) || defined(AMREX_USE_GPU)
#error This is a serial test only.
#endif

using namespace std;

void read_precomp_file(string input_file, int nruns, vector<vector<amrex::Real>> &file_wgts, vector<vector<int>> &file_ranks) {
    cout << input_file << endl;
    // Read the weights input file
    ifstream file(input_file);
    if (!file.is_open()) {
        amrex::Print() << "Error opening file: " << input_file << endl;
        return;
    }

    int i = 0;
    string weights, ranks;
    while(i <= nruns && getline(file, weights) && getline(file, ranks)) {
        // amrex::Print() << "Run " << i << ": Weights: " << weights << ", Ranks: " << ranks << endl;
        file_wgts.push_back(vector<amrex::Real>());
        file_ranks.push_back(vector<int>());

        stringstream ss(weights);
        amrex::Real value;
        while (ss >> value) {
            file_wgts[i].push_back(value);
            if (ss.peek() == ',') {
                ss.ignore();
            }
        }

        amrex::Print() << "Run " << i << ": Weights size: " << file_wgts[i].size() << endl;

        stringstream ss_ranks(ranks);
        int rank;
        while (ss_ranks >> rank) {
            file_ranks[i].push_back(rank);
            if (ss_ranks.peek() == ',') {
                ss_ranks.ignore();
            }
        }
        i++;
    }

    file.close();
}

void lb_real_data(string file, amrex::BoxArray &ba, int nnodes, int nranks, int nruns, int ranks_per_node) {
    vector<vector<amrex::Real>> file_wgts;
    vector<vector<int>> file_ranks;
    read_precomp_file(file, nruns, file_wgts, file_ranks);

    // Variables to store the load balancing distribution for each algorithm
    vector<int> original_map, knapsack_map, kk_map, sfc_map, sfc_knapsack_map, painter_knapsack_map, sfc_painter_map;
    vector<int> prev_original_map, prev_knapsack_map, prev_kk_map, prev_sfc_map, prev_sfc_knapsack_map, prev_painter_knapsack_map, prev_sfc_painter_map;

    amrex::Real og_eff = 0.0, prev_og_eff = 0.0, k_eff = 0.0, prev_k_eff = 0.0, kk_eff = 0.0, prev_kk_eff = 0.0, sfc_eff = 0.0, prev_sfc_eff = 0.0, sfc_knapsack_eff = 0.0, prev_sfc_knapsack_eff = 0.0, prev_sfc_painter_eff = 0.0, sfc_painter_eff = 0.0, prev_painter_knapsack_eff = 0.0, painter_knapsack_eff = 0.0;
        double time_start = amrex::second();
        int nmax = numeric_limits<int>::max();
        vector<amrex::Long> bytes;
        int node_size = 0;
        amrex::Real ratio;



    for(int run_id = 0; run_id <= nruns; run_id++) {
        amrex::Print() << "Run " << run_id << "\n";

        if (file_wgts[run_id].empty() || file_ranks[run_id].empty()) {
            amrex::Print() << "Error: No data read from file." << endl;
            return;
        }

        amrex::Print() << "File weights " << file_wgts.size() << " and ranks " << file_ranks.size() << endl;

        if (file_wgts[run_id].size() != ba.size() || file_ranks[run_id].size() != ba.size()) {
            amrex::Print() << "File weights " << file_wgts[run_id].size() << " and ranks " << file_ranks[run_id].size() 
                           << " do not match the number of boxes " << ba.size() << " and ranks " << nranks << "." << endl;
            // return;
        }

        vector<amrex::Long> wgts(ba.size());
        wgts = scale_wgts(file_wgts[run_id]);

        // Calculate the load balancing efficiency of the read weights
        amrex::Real original_efficiency = 0.0;
        vector<amrex::Long> rank_loads(nranks, 0);
        amrex::Long total_weight = 0;

        if (run_id > 0) {
            rank_loads.assign(nranks, 0);
            total_weight = 0;
            prev_original_map = original_map;

            for (int j = 0; j < wgts.size(); ++j) {
                rank_loads[prev_original_map[j]] += wgts[j];
                total_weight += wgts[j];
            }
            amrex::Long max_load = *max_element(rank_loads.begin(), rank_loads.end());
            prev_og_eff = static_cast<amrex::Real>(total_weight) / (nranks * max_load);

            amrex::Print() << "Original previous efficiency: " << prev_og_eff << endl;
            metric_utils_add(MetricUtilsAlgorithms::ORIGINAL, MetricUtilsMetrics::PREVIOUS_EFFICIENCY, prev_og_eff, run_id);

        }

        rank_loads.assign(nranks, 0);
        total_weight = 0;

        original_map = file_ranks[run_id];
        for (int j = 0; j < wgts.size(); ++j) {
            rank_loads[original_map[j]] += wgts[j];
            total_weight += wgts[j];
        }
        amrex::Long max_load = *max_element(rank_loads.begin(), rank_loads.end());
        og_eff = static_cast<amrex::Real>(total_weight) / (nranks * max_load);

        amrex::Print() << "Original efficiency: " << og_eff << endl;
        metric_utils_add(MetricUtilsAlgorithms::ORIGINAL, MetricUtilsMetrics::EFFICIENCY, og_eff, run_id);

        // Add original weight distribution
        for (int rank = 0; rank < nranks; ++rank) {
            metric_utils_add(MetricUtilsAlgorithms::ORIGINAL, MetricUtilsMetrics::WEIGHT, rank_loads[rank], run_id, rank);
        }

        ratio = og_eff / prev_og_eff;
        amrex::Print() << "Original ratio: " << ratio << endl;
        if (ratio > 1.1) {
            metric_utils_add(MetricUtilsAlgorithms::ORIGINAL, MetricUtilsMetrics::USED_EFFICIENCY, og_eff, run_id);
        } else {
            original_map = prev_original_map; // Use previous map if the ratio is not significant
            metric_utils_add(MetricUtilsAlgorithms::ORIGINAL, MetricUtilsMetrics::USED_EFFICIENCY, prev_og_eff, run_id);
        }

        // KNAPSACK ALGORITHM
        // Calculate previous efficiency for knapsack
        if (run_id > 0) {
            rank_loads.assign(nranks, 0);
            total_weight = 0;
            prev_knapsack_map = knapsack_map;

            for (int j = 0; j < wgts.size(); ++j) {
                rank_loads[prev_knapsack_map[j]] += wgts[j];
                total_weight += wgts[j];
            }
            amrex::Long max_load = *max_element(rank_loads.begin(), rank_loads.end());
            prev_k_eff = static_cast<amrex::Real>(total_weight) / (nranks * max_load);

            amrex::Print() << "Knapsack previous efficiency: " << prev_k_eff << endl;
            metric_utils_add(MetricUtilsAlgorithms::KNAPSACK, MetricUtilsMetrics::PREVIOUS_EFFICIENCY, prev_k_eff, run_id);

        }
        
        time_start = amrex::second();
        knapsack_map = KnapSackDoIt(wgts, nranks, k_eff, true, nmax, false, false, run_id, bytes);
        
        ratio = k_eff / prev_k_eff;
        amrex::Print() << "Knapsack ratio: " << ratio << endl;
        if (ratio > 1.1) {
            metric_utils_add(MetricUtilsAlgorithms::KNAPSACK, MetricUtilsMetrics::USED_EFFICIENCY, k_eff, run_id);
        } else {
            knapsack_map = prev_knapsack_map; // Use previous map if the ratio is not significant
            metric_utils_add(MetricUtilsAlgorithms::KNAPSACK, MetricUtilsMetrics::USED_EFFICIENCY, prev_k_eff, run_id);
        }
        amrex::Print()<<" Final Knapsack time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::KNAPSACK, MetricUtilsMetrics::TIME, amrex::second() - time_start, run_id);


        // KARMARKAR-KARP ALGORITHM
        // Calculate previous efficiency for karmarkar-karp
        if (run_id > 0) {
            rank_loads.assign(nranks, 0);
            total_weight = 0;
            prev_kk_map = kk_map;
            
            for (int j = 0; j < wgts.size(); ++j) {
                rank_loads[prev_kk_map[j]] += wgts[j];
                total_weight += wgts[j];
            }
            amrex::Long max_load = *max_element(rank_loads.begin(), rank_loads.end());
            prev_kk_eff = static_cast<amrex::Real>(total_weight) / (nranks * max_load);

            amrex::Print() << "Karmarkar-Karp previous efficiency: " << prev_kk_eff << endl;
            metric_utils_add(MetricUtilsAlgorithms::KARMARKAR_KARP, MetricUtilsMetrics::PREVIOUS_EFFICIENCY, prev_kk_eff, run_id);

        }

        time_start = amrex::second();
        kk_map = KKDoIt(wgts, nranks, kk_eff, false, run_id);

        ratio = kk_eff / prev_kk_eff;
        amrex::Print() << "Karmarkar-Karp ratio: " << ratio << endl;
        if (ratio > 1.1) {
            metric_utils_add(MetricUtilsAlgorithms::KARMARKAR_KARP, MetricUtilsMetrics::USED_EFFICIENCY, k_eff, run_id);
        } else {
            kk_map = prev_kk_map; // Use previous map if the ratio is not significant
            metric_utils_add(MetricUtilsAlgorithms::KARMARKAR_KARP, MetricUtilsMetrics::USED_EFFICIENCY, prev_kk_eff, run_id);
        }
        
        
        amrex::Print()<<" Final Karmarkar-Karp time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::KARMARKAR_KARP, MetricUtilsMetrics::TIME, amrex::second() - time_start, run_id);

        // SFC ALGORITHM
        // Calculate previous efficiency for SFC
        if (run_id > 0) {
            rank_loads.assign(nranks, 0);
            total_weight = 0;
            prev_sfc_map = sfc_map;

            for (int j = 0; j < wgts.size(); ++j) {
                rank_loads[prev_sfc_map[j]] += wgts[j];
                total_weight += wgts[j];
            }
            amrex::Long max_load = *max_element(rank_loads.begin(), rank_loads.end());
            prev_sfc_eff = static_cast<amrex::Real>(total_weight) / (nranks * max_load);

            amrex::Print() << "SFC previous efficiency: " << prev_sfc_eff << endl;
            metric_utils_add(MetricUtilsAlgorithms::SFC, MetricUtilsMetrics::PREVIOUS_EFFICIENCY, prev_sfc_eff, run_id);
        }

        time_start = amrex::second();
        sfc_map = SFCProcessorMapDoIt(ba, wgts, nranks, &sfc_eff, node_size, false, false, run_id, bytes);

        ratio = sfc_eff / prev_sfc_eff;
        amrex::Print() << "SFC ratio: " << ratio << endl;
        if (ratio > 1.1) {
            metric_utils_add(MetricUtilsAlgorithms::SFC, MetricUtilsMetrics::USED_EFFICIENCY, sfc_eff, run_id);
        } else {
            sfc_map = prev_sfc_map; // Use previous map if the ratio is not significant
            metric_utils_add(MetricUtilsAlgorithms::SFC, MetricUtilsMetrics::USED_EFFICIENCY, prev_sfc_eff, run_id);
        }

        amrex::Print()<<" Final SFC time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::SFC, MetricUtilsMetrics::TIME, amrex::second() - time_start, run_id);

        // SFC + KNAPSACK ALGORITHM
        // Calculate previous efficiency for SFC+Knapsack
        if (run_id > 0) {
            rank_loads.assign(nranks, 0);
            total_weight = 0;
            prev_sfc_knapsack_map = sfc_knapsack_map;

            for (int j = 0; j < wgts.size(); ++j) {
                rank_loads[prev_sfc_knapsack_map[j]] += wgts[j];
                total_weight += wgts[j];
            }
            amrex::Long max_load = *max_element(rank_loads.begin(), rank_loads.end());
            prev_sfc_knapsack_eff = static_cast<amrex::Real>(total_weight) / (nranks * max_load);

            amrex::Print() << "SFC+Knapsack previous efficiency: " << prev_sfc_knapsack_eff << endl;
            metric_utils_add(MetricUtilsAlgorithms::SFC_KNAPSACK, MetricUtilsMetrics::PREVIOUS_EFFICIENCY, prev_sfc_knapsack_eff, run_id);
        }

        time_start = amrex::second();
        sfc_knapsack_map = SFCProcessorMapDoItCombined(ba, wgts, nnodes, ranks_per_node, &sfc_eff, &sfc_knapsack_eff, false, false, run_id, bytes);

        ratio = sfc_knapsack_eff / prev_sfc_knapsack_eff;
        amrex::Print() << "SFC + Knapsack ratio: " << ratio << endl;
        if (ratio > 1.1) {
            metric_utils_add(MetricUtilsAlgorithms::SFC_KNAPSACK, MetricUtilsMetrics::USED_EFFICIENCY, sfc_knapsack_eff, run_id);
        } else {
            sfc_knapsack_map = prev_sfc_knapsack_map; // Use previous map if the ratio is not significant
            metric_utils_add(MetricUtilsAlgorithms::SFC_KNAPSACK, MetricUtilsMetrics::USED_EFFICIENCY, prev_sfc_knapsack_eff, run_id);
        }

        amrex::Print()<<" Final SFC+Knapsack_Combined time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::SFC_KNAPSACK, MetricUtilsMetrics::TIME, amrex::second() - time_start, run_id);

        // Calculate previous efficiency for SFC+Painter
        if (run_id > 0) {
            rank_loads.assign(nranks, 0);
            total_weight = 0;
            prev_sfc_painter_map = sfc_painter_map;

            for (int j = 0; j < wgts.size(); ++j) {
                rank_loads[prev_sfc_painter_map[j]] += wgts[j];
                total_weight += wgts[j];
            }
            amrex::Long max_load = *max_element(rank_loads.begin(), rank_loads.end());
            prev_sfc_painter_eff = static_cast<amrex::Real>(total_weight) / (nranks * max_load);

            amrex::Print() << "SFC+Painter previous efficiency: " << prev_sfc_painter_eff << endl;
            metric_utils_add(MetricUtilsAlgorithms::SFC_PAINTER, MetricUtilsMetrics::PREVIOUS_EFFICIENCY, prev_sfc_painter_eff, run_id);
        }

        time_start = amrex::second();
        sfc_painter_map = painterPartition(ba, wgts, nranks, sfc_painter_eff, run_id, true);

        ratio = sfc_painter_eff / prev_sfc_painter_eff;
        amrex::Print() << "SFC+Painter ratio: " << ratio << endl;
        if (ratio > 1.1) {
            metric_utils_add(MetricUtilsAlgorithms::SFC_PAINTER, MetricUtilsMetrics::USED_EFFICIENCY, sfc_painter_eff, run_id);
        } else {
            sfc_painter_map = prev_sfc_painter_map; // Use previous map if the ratio is not significant
            metric_utils_add(MetricUtilsAlgorithms::SFC_PAINTER, MetricUtilsMetrics::USED_EFFICIENCY, prev_sfc_painter_eff, run_id);
        }

        amrex::Print()<<" Final SFC+Painter time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::SFC_PAINTER, MetricUtilsMetrics::TIME, amrex::second() - time_start, run_id);

        // PAINTER + KNAPSACK ALGORITHM
        // Calculate previous efficiency for Painter+Knapsack
        if (run_id > 0) {
            rank_loads.assign(nranks, 0);
            total_weight = 0;
            prev_painter_knapsack_map = painter_knapsack_map;

            for (int j = 0; j < wgts.size(); ++j) {
                rank_loads[prev_painter_knapsack_map[j]] += wgts[j];
                total_weight += wgts[j];
            }
            amrex::Long max_load = *max_element(rank_loads.begin(), rank_loads.end());
            prev_painter_knapsack_eff = static_cast<amrex::Real>(total_weight) / (nranks * max_load);

            amrex::Print() << "Painter+Knapsack previous efficiency: " << prev_painter_knapsack_eff << endl;
            metric_utils_add(MetricUtilsAlgorithms::PAINTER_KNAPSACK, MetricUtilsMetrics::PREVIOUS_EFFICIENCY, prev_painter_knapsack_eff, run_id);
        }

        time_start = amrex::second();
        painter_knapsack_map = SFCProcessorMapDoItCombinedPainter(ba, wgts, nnodes, ranks_per_node, sfc_eff, painter_knapsack_eff, false, false, run_id, bytes);

        ratio = painter_knapsack_eff / prev_painter_knapsack_eff;
        amrex::Print() << "Painter+Knapsack ratio: " << ratio << endl;
        if (ratio > 1.1) {
            metric_utils_add(MetricUtilsAlgorithms::PAINTER_KNAPSACK, MetricUtilsMetrics::USED_EFFICIENCY, painter_knapsack_eff, run_id);
        } else {
            painter_knapsack_map = prev_painter_knapsack_map; // Use previous map if the ratio is not significant
            metric_utils_add(MetricUtilsAlgorithms::PAINTER_KNAPSACK, MetricUtilsMetrics::USED_EFFICIENCY, prev_painter_knapsack_eff, run_id);
        }
        
        amrex::Print()<<" Final painter+Knapsack_Combined time: " << amrex::second() - time_start << endl;
        metric_utils_add(MetricUtilsAlgorithms::PAINTER_KNAPSACK, MetricUtilsMetrics::TIME, amrex::second() - time_start, run_id);
    }

}

void lb_generated_data(amrex::BoxArray &ba, int nnodes, int nranks, int nruns, int ranks_per_node, amrex::Real mean, amrex::Real stdev) {
    amrex::ResetRandomSeed(rand());

    int nmax = numeric_limits<int>::max();

    vector<amrex::Long> bytes;
    vector<amrex::Long> wgts(ba.size());

    for (int r = 1; r<=nruns; r++) {
        amrex::Print() << "\n=== Starting Run " << r << " ===\n";
       
        amrex::ResetRandomSeed(rand());
        vector<amrex::Real> wgts_tmp(ba.size());

        for (int i = 0; i < ba.size(); ++i) {
            wgts_tmp[i] = amrex::RandomNormal(mean, stdev);
        }
        
        wgts = scale_wgts(wgts_tmp);        

        amrex::Real sfc_eff = 0.0, knapsack_eff = 0.0, kk_eff = 0.0, k_eff = 0.0, s_eff = 0.0, sfc_painter_eff = 0.0;
        int node_size = 0;
        double time_start=0;

        time_start = amrex::second();
        vector<int> k_dmap = KnapSackDoIt(wgts, nranks, k_eff, true, nmax, false, false, r, bytes);
        amrex::Print()<<" Final Knapsack time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::KNAPSACK, MetricUtilsMetrics::TIME, amrex::second() - time_start, r);

        time_start = amrex::second();
        vector<int> kk_dmap = KKDoIt(wgts, nranks, kk_eff, false, r);
        amrex::Print()<<" Final Karmarkar-Karp time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::KARMARKAR_KARP, MetricUtilsMetrics::TIME, amrex::second() - time_start, r);

        time_start = amrex::second();
        vector<int> s_dmap = SFCProcessorMapDoIt(ba, wgts, nranks, &s_eff, node_size, false, false, r, bytes);
        amrex::Print()<<" Final SFC time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::SFC, MetricUtilsMetrics::TIME, amrex::second() - time_start, r);

        time_start = amrex::second();
        vector<int> vec=painterPartition(ba,wgts,nranks,sfc_painter_eff,r);
        amrex::Print()<<" Final SFC+Painter time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::SFC_PAINTER, MetricUtilsMetrics::TIME, amrex::second() - time_start, r);

        time_start = amrex::second();
        vector<int> sfc_knapsack_dmap = SFCProcessorMapDoItCombined(ba, wgts, nnodes, ranks_per_node, &sfc_eff, &knapsack_eff, false, false, r, bytes);
        amrex::Print()<<" Final SFC+Knapsack_Combined time: " << amrex::second() - time_start << endl<<endl;
        metric_utils_add(MetricUtilsAlgorithms::SFC_KNAPSACK, MetricUtilsMetrics::TIME, amrex::second() - time_start, r);

        time_start = amrex::second();
        vector<int> painter_knapsack_dmap = SFCProcessorMapDoItCombinedPainter(ba, wgts, nnodes, ranks_per_node, sfc_eff, knapsack_eff, false, false, r, bytes);
        amrex::Print()<<" Final painter+Knapsack_Combined time: " << amrex::second() - time_start << endl;
        metric_utils_add(MetricUtilsAlgorithms::PAINTER_KNAPSACK, MetricUtilsMetrics::TIME, amrex::second() - time_start, r);

        amrex::Print() << "\n=== End of Run " << r << " ===\n";
        amrex::Print() << "======================================\n\n";
    }
}

int main(int argc, char* argv[]) {
    amrex::Initialize(argc, argv);
    
    // BL_PROFILE("main");

    amrex::IntVect d_size, max_grid_size, nghost, periodicity;
    int nodes, ranks_per_node;
    int steps = 0;
    int intervals = 0; // total steps that the application will run and the intervals when should do the load balancing
    int nruns = 0;
    int print_threshold = 0;
    bool debug = false;
    int amrex_version;
    amrex::Real mean = 0.0;
    amrex::Real stdev = 0.0;
    string file = "";
    string test_case = "";
    {
        amrex::ParmParse pp;
        pp.get("name", test_case);
        pp.get("domain", d_size);
        pp.get("max_grid_size", max_grid_size);
        pp.get("nghost", nghost);
        pp.get("periodicity", periodicity);
        pp.get("nnodes", nodes);
        pp.query("nruns", nruns);
        pp.get("ranks_per_node", ranks_per_node);
        pp.query("steps", steps);
        pp.query("intervals", intervals);
        pp.query("print_threshold", print_threshold);
        pp.query("debug", debug);
        pp.query("file", file);
        pp.get("amrex.v", amrex_version);
        pp.query("mean", mean);
        pp.query("stdev", stdev);
    }

    amrex::Print() << "Starting running " << test_case << " running with " << nodes << " nodes and " << ranks_per_node << " ranks per node." << endl;
    if (steps > 0) {
        amrex::Print() << "Total steps: " << steps << ", intervals: " << intervals << " = " << steps/intervals << " runs" << endl;
    } else {
        amrex::Print() << "Total runs: " << nruns << endl;
    }

    // Create the BoxArray domain
    amrex::Box domain(amrex::IntVect{0}, (d_size -= 1));
    amrex::BoxArray ba(domain);
    ba.maxSize(max_grid_size);

    int nranks = nodes * ranks_per_node;

    if (file != "") {
        amrex::Print() << "Reading precomputed weights and ranks from file: " << file << endl;
        lb_real_data(file, ba, nodes, nranks, steps/intervals, ranks_per_node);
    } else {
        amrex::Print() << "Generating random weights and ranks." << endl;
        lb_generated_data(ba, nodes, nranks, nruns, ranks_per_node, mean, stdev);
    }

    metric_utils_dump();
    amrex::Finalize();
}