#include <AMReX.H>
#include <AMReX_Random.H>
#include <AMReX_MultiFab.H>
#include <AMReX_ParmParse.H>
#include <AMReX_ParallelDescriptor.H>
#include <random>
#include <string>
#include <cstring>
#include <time.h>  
#include <omp.h>
#include <fstream>
#include <numeric>
#include <cassert>
#include <map>
#include <tuple>
#include <set>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>

#include "Util.H"
#include "Knapsack.H"
#include "SFC.H"
#include "SFC_knapsack.H"
#include "painterPartition.H"
#include "HilbertSFC.H"
#include "HilbertDirection.H"

using namespace amrex;
void main_main();

int main(int argc, char* argv[]) {
    amrex::Initialize(argc, argv);
    main_main();
    amrex::Finalize();
}
bool are_face_neighbors(const amrex::Box& a, const amrex::Box& b);
// Structure to accumulate metrics across runs
struct MetricsAccumulator {
    double total_efficiency = 0.0;
    int total_edges = 0;
    int total_local_edges = 0;
    int total_cut_edges = 0;
    std::map<int, long long> total_halo_volumes;
    int run_count = 0;
    
    void add_efficiency(double eff) {
        total_efficiency += eff;
    }
    
    void add_graph_metrics(int edges, int local, int cut) {
        total_edges += edges;
        total_local_edges += local;
        total_cut_edges += cut;
    }
    
    void add_halo_volumes(const std::map<int, long long>& volumes) {
        for (const auto& [rank, volume] : volumes) {
            total_halo_volumes[rank] += volume;
        }
    }
    
    void increment_run() { run_count++; }
    
    double get_avg_efficiency() const { 
        return run_count > 0 ? total_efficiency / run_count : 0.0; 
    }
    
    double get_avg_cut_fraction() const {
        return total_edges > 0 ? (double)total_cut_edges / total_edges : 0.0;
    }
    
    void write_averaged_graph_cuts(const std::string& filename) const {
        std::ofstream ofs(filename);
        ofs << "TotalEdges " << total_edges / run_count << "\n";
        ofs << "LocalEdges " << total_local_edges / run_count << "\n";
        ofs << "CutEdges " << total_cut_edges / run_count << "\n";
        ofs << "LocalFraction " << (double)total_local_edges / total_edges << "\n";
        ofs << "CutFraction " << (double)total_cut_edges / total_edges << "\n";
        ofs.close();
    }
    
    void write_averaged_halo_exchange(const std::string& filename) const {
        std::ofstream outfile(filename);
        for (const auto& [rank, total_volume] : total_halo_volumes) {
            long long avg_volume = run_count > 0 ? total_volume / run_count : 0;
            outfile << "Neighbor rank: " << rank << " Halo exchange volume: " << avg_volume << std::endl;
        }
        outfile.close();
    }
};

// Modified halo exchange function that returns volumes instead of writing directly
std::map<int, long long> GetHaloExchangeVolumes(amrex::MultiFab& mf) {
    mf.FillBoundary();
    
    const auto& dm = mf.DistributionMap();
    const auto& ba = mf.boxArray();
    std::map<int, long long> halo_exchange_volume;

    for (amrex::MFIter mfi(mf); mfi.isValid(); ++mfi) {
        const amrex::Box& valid_box = mfi.validbox();
        const amrex::Box& grown_box = mfi.growntilebox();
        int my_rank = amrex::ParallelDescriptor::MyProc();

        for (int j = 0; j < ba.size(); ++j) {
            if (dm[j] == my_rank) continue;
            const amrex::Box& neighbor_box = ba[j];
            amrex::Box overlap = grown_box & neighbor_box;
            if (!overlap.isEmpty() && !valid_box.contains(overlap)) {
                halo_exchange_volume[dm[j]] += overlap.numPts();
            }
        }
    }
    return halo_exchange_volume;
}

// Modified graph cut function that returns metrics instead of writing directly
std::tuple<int, int, int> GetGraphCutMetrics(const amrex::BoxArray& ba, const amrex::DistributionMapping& dm) {
    int nboxes = ba.size();
    int cut_edges = 0, local_edges = 0, total_edges = 0;
    std::set<std::pair<int, int>> counted_edges;

    for (int i = 0; i < nboxes; ++i) {
        const amrex::Box& box_i = ba[i];
        int rank_i = dm[i];
        for (int j = 0; j < nboxes; ++j) {
            if (i == j) continue;
            const amrex::Box& box_j = ba[j];
            int rank_j = dm[j];
            if (are_face_neighbors(box_i, box_j)) {
                auto edge = std::minmax(i, j);
                if (counted_edges.count(edge)) continue;
                counted_edges.insert(edge);
                total_edges++;
                if (rank_i == rank_j) local_edges++;
                else cut_edges++;
            }
        }
    }
    return std::make_tuple(total_edges, local_edges, cut_edges);
}

// Keep original functions for backward compatibility
void LogHaloExchange(amrex::MultiFab& mf, const std::string& filename) {
    auto volumes = GetHaloExchangeVolumes(mf);
    std::ofstream outfile(filename);
    for (const auto& [rank, volume] : volumes) {
        outfile << "Neighbor rank: " << rank << " Halo exchange volume: " << volume << std::endl;
    }
    outfile.close();
}

bool are_face_neighbors(const amrex::Box& a, const amrex::Box& b) {
    int n_touch = 0;
    for (int d = 0; d < AMREX_SPACEDIM; ++d) {
        if (a.bigEnd(d) + 1 == b.smallEnd(d) || b.bigEnd(d) + 1 == a.smallEnd(d)) {
            n_touch++;
        } else if (a.bigEnd(d) < b.smallEnd(d) - 1 || b.bigEnd(d) < a.smallEnd(d) - 1) {
            return false;
        }
    }
    return n_touch == 1;
}

void ComputeGraphCutMetrics(const amrex::BoxArray& ba, const amrex::DistributionMapping& dm, const std::string& filename) {
    auto [total_edges, local_edges, cut_edges] = GetGraphCutMetrics(ba, dm);
    std::ofstream ofs(filename);
    ofs << "TotalEdges " << total_edges << "\n";
    ofs << "LocalEdges " << local_edges << "\n";
    ofs << "CutEdges " << cut_edges << "\n";
    ofs << "LocalFraction " << (double)local_edges / total_edges << "\n";
    ofs << "CutFraction " << (double)cut_edges / total_edges << "\n";
    ofs.close();
}

void output_distribution_data(const amrex::BoxArray& ba, 
                            const std::vector<int>& dmap,
                            const std::vector<Long>& costs,
                            const std::string& filename,
                            const std::string& algorithm_name) {
    if (amrex::ParallelDescriptor::MyProc() != 0) return;

    AMREX_ALWAYS_ASSERT(ba.size() == dmap.size());
    AMREX_ALWAYS_ASSERT(ba.size() == costs.size());

    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        amrex::Print() << "Failed to open output file: " << filename << std::endl;
        return;
    }

    outfile << "# Distribution mapping data for " << algorithm_name << "\n";
    outfile << "# Format: box_id rank x y z cost\n";
    
    amrex::Print() << "\nWriting " << algorithm_name << " distribution to " << filename << "\n";
    amrex::Print() << "Number of boxes: " << ba.size() << "\n";
    amrex::Print() << "Distribution size: " << dmap.size() << "\n";
    amrex::Print() << "Costs size: " << costs.size() << "\n";
    
    std::map<int, std::vector<std::tuple<int, int, int, int, Long>>> rank_boxes;
    
    for (int i = 0; i < ba.size(); ++i) {
        const amrex::Box& box = ba[i];
        for (int z = box.smallEnd(2); z <= box.bigEnd(2); ++z) {
            for (int y = box.smallEnd(1); y <= box.bigEnd(1); ++y) {
                for (int x = box.smallEnd(0); x <= box.bigEnd(0); ++x) {
                    rank_boxes[dmap[i]].push_back(std::make_tuple(i, x, y, z, costs[i]));
                }
            }
        }
    }
    
    amrex::Print() << "Rank distribution:\n";
    for (const auto& [rank, boxes] : rank_boxes) {
        amrex::Print() << "Rank " << rank << ": " << boxes.size() << " cells\n";
    }
    
    for (const auto& [rank, boxes] : rank_boxes) {
        for (const auto& [box_id, x, y, z, cost] : boxes) {
            outfile << box_id << " " << rank << " " 
                   << x << " " << y << " " << z << " "
                   << cost << "\n";
        }
    }
    
    outfile.close();
    amrex::Print() << "Finished writing " << filename << "\n\n";
}

void main_main() {
    BL_PROFILE("main");

    int ncomp;
    double scaling = 0.0;
    std::string name = "fb";
    int nbins, nnodes, ranks_per_node;
    Real mean, stdev;
    int nruns = 1;
    IntVect d_size, mgs, nghost, piv;
    {
        ParmParse pp;
        pp.get("domain", d_size);
        pp.get("max_grid_size", mgs);
        pp.get("ncomp", ncomp);
        pp.get("nghost", nghost);
        pp.get("periodicity", piv);
        pp.get("nnodes", nnodes); 
        pp.get("mean", mean);
        pp.get("stdev",stdev);
        pp.get("ranks_per_node", ranks_per_node);  
        pp.query("nruns",nruns);
        pp.query("name", name);
        pp.query("scaling", scaling);
    }
    amrex::Print() << "Mean: " << mean << std::endl;
    amrex::Print() << "Stdev: " << stdev << std::endl;
    amrex::Print() << "No of Run: " << nruns << std::endl;

    amrex::ResetRandomSeed(rand());

    int nmax = std::numeric_limits<int>::max();
    Real k_eff = 0.0;
    Real s_eff = 0.0;

    Box domain(IntVect{0}, (d_size -= 1));
    BoxArray ba(domain);
    ba.maxSize(mgs);
    amrex::Print() << "domain       = " << domain << '\n';
    amrex::Print() << "max_grid_size= " << mgs    << '\n';
    amrex::Print() << "boxes.size() = " << ba.size() << '\n';

    int nitems = ba.size();
    int nranks = nnodes * ranks_per_node;

    amrex::Print() << "Number of nodes: " << nnodes << std::endl;
    amrex::Print() << "Number of boxes: " << nitems << std::endl;
    amrex::Print() << "Ranks per node: " << ranks_per_node << std::endl;
    amrex::Print() << "Number of ranks: " << nranks << std::endl;

    std::vector<amrex::Real> wgts(nitems);
    std::vector<Long> bytes;

    // Initialize metric accumulators
    MetricsAccumulator knapsack_metrics, sfc_metrics, hilbert_metrics, hilbert_painter_metrics;
    
    // Store the last run's distribution data for output
    std::vector<int> final_k_dmap, final_s_dmap, final_hilbert_dmap, final_hilbert_painter_dmap;
    std::vector<Long> final_scaled_wgts;

    for (int r = 0; r < nruns; r++) {
        amrex::Print() << "\n=== Starting Run " << r + 1 << " ===\n";
        amrex::ResetRandomSeed(rand());

        for (int i = 0; i < nitems; ++i) {
            wgts[i] = amrex::RandomNormal(mean, stdev);
            amrex::Print() << wgts[i] << " , ";
        }
        std::vector<Long> scaled_wgts = scale_wgts(wgts);
        amrex::Print() << " Scaled Weights: ";
        for (int i = 0; i < nitems; ++i) {
            amrex::Print() << scaled_wgts[i] << " , ";
        }

        amrex::Real sfc_eff = 0.0, knapsack_eff = 0.0, hilbertsfc_eff = 0.0, hilbert_painter_eff = 0.0, x_hilbertsfc_eff = 0.0;
        int node_size = 0;
        double time_start = 0;
        int ng = nghost[0];

        // KNAPSACK
        time_start = amrex::second();
        std::vector<int> k_dmap = KnapSackDoIt(scaled_wgts, nranks, k_eff, true, nmax, true, false, bytes);
        amrex::Print() << " Final Knapsack time: " << amrex::second() - time_start << std::endl << std::endl;
        
        // Accumulate knapsack metrics
        knapsack_metrics.add_efficiency(k_eff);
        amrex::MultiFab mf_knapsack(ba, amrex::DistributionMapping(amrex::Vector<int>(k_dmap.begin(), k_dmap.end())), 1, ng);
        auto knapsack_halo = GetHaloExchangeVolumes(mf_knapsack);
        knapsack_metrics.add_halo_volumes(knapsack_halo);
        auto [k_total, k_local, k_cut] = GetGraphCutMetrics(ba, amrex::DistributionMapping(amrex::Vector<int>(k_dmap.begin(), k_dmap.end())));
        knapsack_metrics.add_graph_metrics(k_total, k_local, k_cut);

        // SFC
        time_start = amrex::second();
        std::vector<int> s_dmap = SFCProcessorMapDoIt(ba, scaled_wgts, nranks, &s_eff, node_size, true, false, bytes);
        amrex::Print() << " Final SFC time: " << amrex::second() - time_start << std::endl << std::endl;
        
        // Accumulate SFC metrics
        sfc_metrics.add_efficiency(s_eff);
        amrex::MultiFab mf_sfc(ba, amrex::DistributionMapping(amrex::Vector<int>(s_dmap.begin(), s_dmap.end())), 1, ng);
        auto sfc_halo = GetHaloExchangeVolumes(mf_sfc);
        sfc_metrics.add_halo_volumes(sfc_halo);
        auto [s_total, s_local, s_cut] = GetGraphCutMetrics(ba, amrex::DistributionMapping(amrex::Vector<int>(s_dmap.begin(), s_dmap.end())));
        sfc_metrics.add_graph_metrics(s_total, s_local, s_cut);

        // SFC + Painter
        time_start = amrex::second();
        std::vector<int> vec = painterPartition(ba, scaled_wgts, nranks);
        amrex::Print() << " Final SFC+Painter time: " << amrex::second() - time_start << std::endl << std::endl;

        time_start = amrex::second();
        std::vector<int> sfc_knapsack_dmap = SFCProcessorMapDoItCombined(ba, scaled_wgts, nnodes, ranks_per_node, &sfc_eff, &knapsack_eff, true, false, bytes);
        amrex::Print() << " Final SFC+Knapsack_Combined time: " << amrex::second() - time_start << std::endl << std::endl;
        
        time_start = amrex::second();
        std::vector<int> painter_knapsack_dmap = SFCProcessorMapDoItCombinedPainter(ba, scaled_wgts, nnodes, ranks_per_node, &sfc_eff, &knapsack_eff, true, false, bytes);
        amrex::Print() << " Final painter+Knapsack_Combined time: " << amrex::second() - time_start << std::endl;

        // HILBERT
        time_start = amrex::second();
        std::vector<int> hilbertsfc_dmap = HilbertProcessorMapDoIt(ba, scaled_wgts, nranks, &hilbertsfc_eff, ranks_per_node, true, false, bytes);
        std::vector<int> x_hilbertsfc_dmap = HilbertProcessorMapDoItWithDirectionTesting(ba, scaled_wgts, nranks, &x_hilbertsfc_eff, node_size, true, false, bytes);
        amrex::Print() << " Final Hilbert SFC time: " << amrex::second() - time_start << std::endl;
        
        // Accumulate Hilbert metrics
        hilbert_metrics.add_efficiency(hilbertsfc_eff);
        amrex::MultiFab mf_hilbert(ba, amrex::DistributionMapping(amrex::Vector<int>(hilbertsfc_dmap.begin(), hilbertsfc_dmap.end())), 1, ng);
        auto hilbert_halo = GetHaloExchangeVolumes(mf_hilbert);
        hilbert_metrics.add_halo_volumes(hilbert_halo);
        auto [h_total, h_local, h_cut] = GetGraphCutMetrics(ba, amrex::DistributionMapping(amrex::Vector<int>(hilbertsfc_dmap.begin(), hilbertsfc_dmap.end())));
        hilbert_metrics.add_graph_metrics(h_total, h_local, h_cut);

        // Hilbert + Painter
        time_start = amrex::second();
        std::vector<int> hilbert_painter_dmap = painterPartitionHilbert(ba, scaled_wgts, nranks);
        amrex::Print() << " Final Hilbert+Painter time: " << amrex::second() - time_start << std::endl;
        
        {
            std::vector<amrex::Long> loads(nranks, 0);
            for (int i = 0; i < scaled_wgts.size(); ++i) {
                loads[hilbert_painter_dmap[i]] += scaled_wgts[i];
            }
            
            amrex::Long max_load = *std::max_element(loads.begin(), loads.end());
            amrex::Long total_load = std::accumulate(loads.begin(), loads.end(), 0L);
            
            if (max_load > 0) {
                hilbert_painter_eff = static_cast<amrex::Real>(total_load) / (nranks * max_load);
            }
        }

        // Accumulate Hilbert+Painter's metrics
        hilbert_painter_metrics.add_efficiency(hilbert_painter_eff);
        amrex::MultiFab mf_hilbert_painter(ba, amrex::DistributionMapping(amrex::Vector<int>(hilbert_painter_dmap.begin(), hilbert_painter_dmap.end())), 1, ng);
        auto hilbert_painter_halo = GetHaloExchangeVolumes(mf_hilbert_painter);
        hilbert_painter_metrics.add_halo_volumes(hilbert_painter_halo);
        auto [hp_total, hp_local, hp_cut] = GetGraphCutMetrics(ba, amrex::DistributionMapping(amrex::Vector<int>(hilbert_painter_dmap.begin(), hilbert_painter_dmap.end())));
        hilbert_painter_metrics.add_graph_metrics(hp_total, hp_local, hp_cut);


        // Increment run counters
        knapsack_metrics.increment_run();
        sfc_metrics.increment_run();
        hilbert_metrics.increment_run();
        hilbert_painter_metrics.increment_run();

        // Store final run data for distribution output
        if (r == nruns - 1) {
            final_k_dmap = k_dmap;
            final_s_dmap = s_dmap;
            final_hilbert_dmap = hilbertsfc_dmap;
            final_hilbert_painter_dmap = hilbert_painter_dmap;
            final_scaled_wgts = scaled_wgts;
        }

        amrex::Print() << "\n=== End of Run " << r + 1 << " ===\n";
        amrex::Print() << "======================================\n\n";
    }

    // Write averaged results
    amrex::Print() << "\n=== WRITING AVERAGED RESULTS ===\n";
    
    // Write distribution data from final run
    output_distribution_data(ba, final_k_dmap, final_scaled_wgts, "LBC_knapsack.txt", "Knapsack");
    output_distribution_data(ba, final_s_dmap, final_scaled_wgts, "LBC_sfc.txt", "SFC");
    output_distribution_data(ba, final_hilbert_dmap, final_scaled_wgts, "LBC_hilbert.txt", "Hilbert SFC");
    output_distribution_data(ba, final_hilbert_painter_dmap, final_scaled_wgts, "LBC_hilbert_painter.txt", "Hilbert Painter");
    
    // Write averaged metrics
    knapsack_metrics.write_averaged_graph_cuts("LBC_knapsack_graph_cut.txt");
    knapsack_metrics.write_averaged_halo_exchange("LBC_knapsack_halo_exchange.txt");
    
    sfc_metrics.write_averaged_graph_cuts("LBC_sfc_graph_cut.txt");
    sfc_metrics.write_averaged_halo_exchange("LBC_sfc_halo_exchange.txt");
    
    hilbert_metrics.write_averaged_graph_cuts("LBC_hilbert_graph_cut.txt");
    hilbert_metrics.write_averaged_halo_exchange("LBC_hilbert_halo_exchange.txt");

    hilbert_painter_metrics.write_averaged_graph_cuts("LBC_hilbert_painter_graph_cut.txt");
    hilbert_painter_metrics.write_averaged_halo_exchange("LBC_hilbert_painter_halo_exchange.txt");

    // Print summary
    amrex::Print() << "\n=== AVERAGED PERFORMANCE SUMMARY ===\n";
    amrex::Print() << "Knapsack - Avg Efficiency: " << knapsack_metrics.get_avg_efficiency() 
                   << ", Avg Cut Fraction: " << knapsack_metrics.get_avg_cut_fraction() << std::endl;
    amrex::Print() << "SFC - Avg Efficiency: " << sfc_metrics.get_avg_efficiency() 
                   << ", Avg Cut Fraction: " << sfc_metrics.get_avg_cut_fraction() << std::endl;
    amrex::Print() << "Hilbert - Avg Efficiency: " << hilbert_metrics.get_avg_efficiency() 
                   << ", Avg Cut Fraction: " << hilbert_metrics.get_avg_cut_fraction() << std::endl;
    amrex::Print() << "Hilbert + Painter - Avg Efficiency: " << hilbert_painter_metrics.get_avg_efficiency()
                   << ", Avg Cut Fraction: " << hilbert_painter_metrics.get_avg_cut_fraction() << std::endl;
}