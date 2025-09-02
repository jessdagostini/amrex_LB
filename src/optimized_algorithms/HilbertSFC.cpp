#include <AMReX_Extension.H>
#include <AMReX_IntVect.H>
#include <AMReX_Print.H>
#include <AMReX_ParallelContext.H>
#include <AMReX.H>
#include <array>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <numeric>

#include "HilbertSFC.H"
#include "Knapsack.H"
#include "LeastUsed.H"


struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        auto h1 = std::hash<int>{}(p.first);
        auto h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

namespace {
    // Helper function to get sign of a number
    inline int sgn(int x) {
        return (x < 0) ? -1 : ((x > 0) ? 1 : 0);
    }

    // Generate 3D Gilbert curve points (port of your Python code)
    void generate3d_points(int x, int y, int z,
                          int ax, int ay, int az,
                          int bx, int by, int bz,
                          int cx, int cy, int cz,
                          std::vector<std::tuple<int, int, int>>& points) {
        
        int w = std::abs(ax + ay + az);
        int h = std::abs(bx + by + bz);
        int d = std::abs(cx + cy + cz);

        int dax = sgn(ax), day = sgn(ay), daz = sgn(az);
        int dbx = sgn(bx), dby = sgn(by), dbz = sgn(bz);
        int dcx = sgn(cx), dcy = sgn(cy), dcz = sgn(cz);

        // Base cases for trivial dimensions
        if (h == 1 && d == 1) {
            for (int i = 0; i < w; ++i) {
                points.emplace_back(x, y, z);
                x += dax; y += day; z += daz;
            }
            return;
        }
        if (w == 1 && d == 1) {
            for (int i = 0; i < h; ++i) {
                points.emplace_back(x, y, z);
                x += dbx; y += dby; z += dbz;
            }
            return;
        }
        if (w == 1 && h == 1) {
            for (int i = 0; i < d; ++i) {
                points.emplace_back(x, y, z);
                x += dcx; y += dcy; z += dcz;
            }
            return;
        }

        // Split dimensions
        int ax2 = ax/2, ay2 = ay/2, az2 = az/2;
        int bx2 = bx/2, by2 = by/2, bz2 = bz/2;
        int cx2 = cx/2, cy2 = cy/2, cz2 = cz/2;

        int w2 = std::abs(ax2 + ay2 + az2);
        int h2 = std::abs(bx2 + by2 + bz2);
        int d2 = std::abs(cx2 + cy2 + cz2);

        // Prefer even steps
        if ((w2 % 2) && (w > 2)) {
            ax2 += dax; ay2 += day; az2 += daz;
        }
        if ((h2 % 2) && (h > 2)) {
            bx2 += dbx; by2 += dby; bz2 += dbz;
        }
        if ((d2 % 2) && (d > 2)) {
            cx2 += dcx; cy2 += dcy; cz2 += dcz;
        }

        // Wide case: split in w only
        if ((2*w > 3*h) && (2*w > 3*d)) {
            generate3d_points(x, y, z,
                            ax2, ay2, az2,
                            bx, by, bz,
                            cx, cy, cz, points);

            generate3d_points(x+ax2, y+ay2, z+az2,
                            ax-ax2, ay-ay2, az-az2,
                            bx, by, bz,
                            cx, cy, cz, points);
        }
        // Do not split in d
        else if (3*h > 4*d) {
            generate3d_points(x, y, z,
                            bx2, by2, bz2,
                            cx, cy, cz,
                            ax2, ay2, az2, points);

            generate3d_points(x+bx2, y+by2, z+bz2,
                            ax, ay, az,
                            bx-bx2, by-by2, bz-bz2,
                            cx, cy, cz, points);

            generate3d_points(x+(ax-dax)+(bx2-dbx),
                            y+(ay-day)+(by2-dby),
                            z+(az-daz)+(bz2-dbz),
                            -bx2, -by2, -bz2,
                            cx, cy, cz,
                            -(ax-ax2), -(ay-ay2), -(az-az2), points);
        }
        // Do not split in h
        else if (3*d > 4*h) {
            generate3d_points(x, y, z,
                            cx2, cy2, cz2,
                            ax2, ay2, az2,
                            bx, by, bz, points);

            generate3d_points(x+cx2, y+cy2, z+cz2,
                            ax, ay, az,
                            bx, by, bz,
                            cx-cx2, cy-cy2, cz-cz2, points);

            generate3d_points(x+(ax-dax)+(cx2-dcx),
                            y+(ay-day)+(cy2-dcy),
                            z+(az-daz)+(cz2-dcz),
                            -cx2, -cy2, -cz2,
                            -(ax-ax2), -(ay-ay2), -(az-az2),
                            bx, by, bz, points);
        }
        // Regular case: split in all w/h/d
        else {
            generate3d_points(x, y, z,
                            bx2, by2, bz2,
                            cx2, cy2, cz2,
                            ax2, ay2, az2, points);

            generate3d_points(x+bx2, y+by2, z+bz2,
                            cx, cy, cz,
                            ax2, ay2, az2,
                            bx-bx2, by-by2, bz-bz2, points);

            generate3d_points(x+(bx2-dbx)+(cx-dcx),
                            y+(by2-dby)+(cy-dcy),
                            z+(bz2-dbz)+(cz-dcz),
                            ax, ay, az,
                            -bx2, -by2, -bz2,
                            -(cx-cx2), -(cy-cy2), -(cz-cz2), points);

            generate3d_points(x+(ax-dax)+bx2+(cx-dcx),
                            y+(ay-day)+by2+(cy-dcy),
                            z+(az-daz)+bz2+(cz-dcz),
                            -cx, -cy, -cz,
                            -(ax-ax2), -(ay-ay2), -(az-az2),
                            bx-bx2, by-by2, bz-bz2, points);

            generate3d_points(x+(ax-dax)+(bx2-dbx),
                            y+(ay-day)+(by2-dby),
                            z+(az-daz)+(bz2-dbz),
                            -bx2, -by2, -bz2,
                            cx2, cy2, cz2,
                            -(ax-ax2), -(ay-ay2), -(az-az2), points);
        }
    }

    std::vector<std::tuple<int, int, int>> gilbert3d(int width, int height, int depth, 
                                                    HilbertDirection direction = HilbertDirection::DEFAULT) {
        std::vector<std::tuple<int, int, int>> points;
        
        // Choose starting orientation based on direction
        switch (direction) {
            case HilbertDirection::X_MAJOR:
                generate3d_points(0, 0, 0,
                                width, 0, 0,
                                0, height, 0,
                                0, 0, depth, points);
                break;
                
            case HilbertDirection::Y_MAJOR:
                generate3d_points(0, 0, 0,
                                0, height, 0,
                                width, 0, 0,
                                0, 0, depth, points);
                break;
                
            case HilbertDirection::Z_MAJOR:
                generate3d_points(0, 0, 0,
                                0, 0, depth,
                                width, 0, 0,
                                0, height, 0, points);
                break;
                
            case HilbertDirection::DEFAULT:
            default:
                // Your original logic
                if (width >= height && width >= depth) {
                    generate3d_points(0, 0, 0,
                                    width, 0, 0,
                                    0, height, 0,
                                    0, 0, depth, points);
                } else if (height >= width && height >= depth) {
                    generate3d_points(0, 0, 0,
                                    0, height, 0,
                                    width, 0, 0,
                                    0, 0, depth, points);
                } else {
                    generate3d_points(0, 0, 0,
                                    0, 0, depth,
                                    width, 0, 0,
                                    0, height, 0, points);
                }
                break;
        }
        
        return points;
    }

} // anonymous namespace

uint64_t coords_to_hilbert(uint32_t x, uint32_t y, uint32_t z, int order, 
                          HilbertDirection direction) {
    uint32_t size = 1 << order;
    
    // Ensure coordinates are within bounds
    x = std::min(x, size - 1);
    y = std::min(y, size - 1);
    z = std::min(z, size - 1);

    // Use lookup table for reasonable orders
    static std::unordered_map<std::pair<int, int>, std::unordered_map<uint64_t, uint64_t>, PairHash> cache;

    static bool first_call = true;
    if (first_call) {
        amrex::Print() << "Building Hilbert lookup table for order " << order << "...\n";
        first_call = false;
    }
    
    std::pair<int, int> cache_key = {order, static_cast<int>(direction)};
    
    if (cache[cache_key].empty() && order <= 8) {  // Only cache up to 256^3
        // Build lookup table using gilbert3d with direction
        auto points = gilbert3d(size, size, size, direction);
        
        // Reserve space for better performance
        cache[cache_key].reserve(points.size());
        
        // Build the coordinate -> index mapping
        for (uint64_t i = 0; i < points.size(); ++i) {
            auto [px, py, pz] = points[i];
            
            // Ensure coordinates fit in our key encoding (10 bits each)
            if (px >= (1 << 10) || py >= (1 << 10) || pz >= (1 << 10)) {
                continue; // Skip invalid coordinates
            }
            
            uint64_t key = (uint64_t(px) << 20) | (uint64_t(py) << 10) | uint64_t(pz);
            cache[cache_key][key] = i;
        }
    }
    
    if (order <= 8) {  // Only use cache for orders <= 8
        // Ensure coordinates fit in 10 bits each for key encoding
        if (x < (1 << 10) && y < (1 << 10) && z < (1 << 10)) {
            uint64_t key = (uint64_t(x) << 20) | (uint64_t(y) << 10) | uint64_t(z);
            auto it = cache[cache_key].find(key);
            if (it != cache[cache_key].end()) {
                return it->second;
            }
        }
    }
    
    // Fallback for large orders or out-of-range coordinates
    return (uint64_t(z) << 42) | (uint64_t(y) << 21) | uint64_t(x);
}

void HilbertDistribute(const std::vector<HilbertSFCToken>& tokens,
                      const std::vector<amrex::Long>& wgts,
                      int nprocs,
                      amrex::Real volpercpu,
                      std::vector<std::vector<int>>& v,
                      bool flag_verbose_mapper) {
    BL_PROFILE("HilbertDistribute()");

    BL_ASSERT(static_cast<int>(v.size()) == nprocs);

    int K = 0;
    amrex::Real totalvol = 0;

    for (int i = 0; i < nprocs; ++i) {
        int cnt = 0;
        amrex::Real vol = 0;

        for (int TSZ = static_cast<int>(tokens.size());
             K < TSZ && (i == (nprocs - 1) || (vol < volpercpu));
             ++K) {
            vol += wgts[tokens[K].m_box];
            ++cnt;
            v[i].push_back(tokens[K].m_box);
        }

        totalvol += vol;

        if ((totalvol / (i + 1)) > volpercpu &&  // Too much for this bin
            cnt > 1 &&                          // More than one box in this bin
            i < nprocs - 1) {                    // Not the last bin
            --K;
            v[i].pop_back();
            totalvol -= wgts[tokens[K].m_box];
        }
    }

#ifdef AMREX_DEBUG
    int cnt = 0;
    for (int i = 0; i < nprocs; ++i) {
        cnt += v[i].size();
    }
    BL_ASSERT(cnt == static_cast<int>(tokens.size()));
#endif
}

std::vector<int>
HilbertProcessorMapDoIt(const amrex::BoxArray& boxes,
                       const std::vector<amrex::Long>& wgts,
                       int nprocs,
                       amrex::Real* eff,
                       int node_size,
                       bool flag_verbose_mapper,
                       bool sort,
                       const std::vector<amrex::Long>& bytes,
                       HilbertDirection direction) {
    if (flag_verbose_mapper) {
        amrex::Print() << "DM: HilbertProcessorMapDoIt called..." << std::endl;
    }

    BL_PROFILE("HilbertProcessorMapDoIt()");

    int nteams = nprocs;
    int nworkers = 1;

#if defined(BL_USE_TEAM)
    nteams = ParallelDescriptor::NTeams();
    nworkers = ParallelDescriptor::TeamSize();
#else
    if (node_size > 0) {
        nteams = nprocs / node_size;
        nworkers = node_size;
        if (nworkers * nteams != nprocs) {
            nteams = nprocs;
            nworkers = 1;
        }
    }
#endif

    if (flag_verbose_mapper) {
        amrex::Print() << "  (nprocs, nteams, nworkers) = ("
                       << nprocs << ", " << nteams << ", " << nworkers << ")\n";
    }

    const int N = boxes.size();
    std::vector<HilbertSFCToken> tokens;
    tokens.reserve(N);
    for (int i = 0; i < N; ++i) {
        const amrex::Box& bx = boxes[i];
        tokens.push_back(makeHilbertSFCToken(i, bx.smallEnd(), direction));
    }

    // Sort tokens along Hilbert curve
    std::sort(tokens.begin(), tokens.end(), HilbertSFCToken::Compare());

    // Split them up as equitably as possible per team
    amrex::Real volperteam = 0;
    for (amrex::Long wt : wgts) {
        volperteam += wt;
    }
    volperteam /= nteams;

    std::vector<std::vector<int>> vec(nteams);
    HilbertDistribute(tokens, wgts, nteams, volperteam, vec);

    tokens.clear();

    std::vector<LIpair> LIpairV;
    LIpairV.reserve(nteams);

    for (int i = 0; i < nteams; ++i) {
        amrex::Long wgt = 0;
        const std::vector<int>& vi = vec[i];
        for (int j = 0, M = vi.size(); j < M; ++j) {
            wgt += wgts[vi[j]];
        }
        LIpairV.push_back(LIpair(wgt, i));
    }

    if (sort) Sort(LIpairV, true);

    if (flag_verbose_mapper) {
        for (const auto &p : LIpairV) {
            amrex::Print() << "  Bucket " << p.second << " contains " << p.first << std::endl;
        }
    }

    amrex::Vector<int> ord;
    amrex::Vector<amrex::Vector<int>> wrkerord;

    if (nteams == nprocs) {
        if (sort) {
            LeastUsedCPUs(nprocs, bytes, ord, flag_verbose_mapper);
        } else {
            ord.resize(nprocs);
            std::iota(ord.begin(), ord.end(), 0);
        }
    } else {
        if (sort) {
            // LeastUsedTeams(ord, wrkerord, nteams, nworkers);
        } else {
            ord.resize(nteams);
            std::iota(ord.begin(), ord.end(), 0);
            wrkerord.resize(nteams);
            for (auto& v : wrkerord) {
                v.resize(nworkers);
                std::iota(v.begin(), v.end(), 0);
            }
        }
    }

    std::vector<int> result(wgts.size());

    for (int i = 0; i < nteams; ++i) {
        const int tid = ord[i];
        const int ivec = LIpairV[i].second;
        const std::vector<int>& vi = vec[ivec];
        const int Nbx = vi.size();

        if (nteams == nprocs) {
            for (int j = 0; j < Nbx; ++j) {
#ifdef AMREX_USE_MPI
                result[vi[j]] = amrex::ParallelContext::local_to_global_rank(tid);
#else
                result[vi[j]] = tid;
#endif
            }
        } else {
            std::vector<amrex::Long> local_wgts;
            for (int j = 0; j < Nbx; ++j) {
                local_wgts.push_back(wgts[vi[j]]);
            }

            std::vector<std::vector<int>> kpres;
            amrex::Real kpeff;
            knapsack(local_wgts, nworkers, kpres, kpeff, true, N);

            std::vector<LIpair> ww;
            for (int w = 0; w < nworkers; ++w) {
                amrex::Long wgt = 0;
                for (std::vector<int>::const_iterator it = kpres[w].begin();
                     it != kpres[w].end(); ++it) {
                    wgt += local_wgts[*it];
                }
                ww.push_back(LIpair(wgt, w));
            }
            Sort(ww, true);

            const amrex::Vector<int>& sorted_workers = wrkerord[i];
            const int leadrank = tid * nworkers;

            for (int w = 0; w < nworkers; ++w) {
                const int cpu = leadrank + sorted_workers[w];
                int ikp = ww[w].second;
                const std::vector<int>& js = kpres[ikp];
                for (std::vector<int>::const_iterator it = js.begin(); it != js.end(); ++it) {
                    result[vi[*it]] = cpu;
                }
            }
        }
    }

    if (eff || flag_verbose_mapper) {
        amrex::Real sum_wgt = 0, max_wgt = 0;
        for (int i = 0; i < nteams; ++i) {
            const amrex::Long W = LIpairV[i].first;
            if (W > max_wgt) max_wgt = W;
            sum_wgt += W;
        }
        amrex::Real efficiency = (sum_wgt / (nteams * max_wgt));
        if (eff) *eff = efficiency;

        if (flag_verbose_mapper) {
            amrex::Print() << "Hilbert SFC efficiency: " << efficiency << '\n';
        }
    }

    return result;
}

// Add this new function to test all directions
std::vector<int>
HilbertProcessorMapDoItWithDirectionTesting(const amrex::BoxArray& boxes,
                                           const std::vector<amrex::Long>& wgts,
                                           int nprocs,
                                           amrex::Real* best_eff,
                                           int node_size,
                                           bool flag_verbose_mapper,
                                           bool sort,
                                           const std::vector<amrex::Long>& bytes) {
    
    if (flag_verbose_mapper) {
        amrex::Print() << "\n=== Testing All Hilbert Directions ===\n";
    }
    
    // Test all 4 directions
    std::vector<HilbertDirection> directions = {
        HilbertDirection::DEFAULT,
        HilbertDirection::X_MAJOR,
        HilbertDirection::Y_MAJOR,
        HilbertDirection::Z_MAJOR
    };
    
    std::vector<std::string> direction_names = {
        "DEFAULT", "X_MAJOR", "Y_MAJOR", "Z_MAJOR"
    };
    
    std::vector<int> best_result;
    amrex::Real best_efficiency = 0.0;
    amrex::Real best_load_variance = 1e9;
    std::string best_direction_name;
    
    // Test each direction
    for (size_t d = 0; d < directions.size(); ++d) {
        if (flag_verbose_mapper) {
            amrex::Print() << "\n--- Testing " << direction_names[d] << " ---\n";
        }
        
        // Run Hilbert with this direction - FIXED: Call the actual function, not itself
        amrex::Real current_eff = 0.0;
        auto current_result = HilbertProcessorMapDoIt(boxes, wgts, nprocs, 
                                                     &current_eff, node_size, 
                                                     false, // Don't print individual results
                                                     sort, bytes, directions[d]);
        
        // Calculate load distribution statistics
        std::vector<amrex::Long> rank_loads(nprocs, 0);
        for (size_t i = 0; i < wgts.size(); ++i) {
            rank_loads[current_result[i]] += wgts[i];
        }
        
        // Calculate variance and other metrics
        amrex::Real mean_load = 0.0;
        amrex::Real max_load = 0.0;
        amrex::Real min_load = 1e9;
        
        for (int i = 0; i < nprocs; ++i) {
            mean_load += rank_loads[i];
            max_load = std::max(max_load, (amrex::Real)rank_loads[i]);
            min_load = std::min(min_load, (amrex::Real)rank_loads[i]);
        }
        mean_load /= nprocs;
        
        amrex::Real variance = 0.0;
        for (int i = 0; i < nprocs; ++i) {
            amrex::Real diff = rank_loads[i] - mean_load;
            variance += diff * diff;
        }
        variance /= nprocs;
        
        amrex::Real load_balance_ratio = min_load / max_load;
        amrex::Real coefficient_of_variation = sqrt(variance) / mean_load;
        
        if (flag_verbose_mapper) {
            amrex::Print() << "  " << direction_names[d] << " Results:\n";
            amrex::Print() << "    Efficiency: " << current_eff << "\n";
            amrex::Print() << "    Load Variance: " << variance << "\n";
            amrex::Print() << "    Load Balance Ratio: " << load_balance_ratio << "\n";
            amrex::Print() << "    Coefficient of Variation: " << coefficient_of_variation << "\n";
            amrex::Print() << "    Max Load: " << max_load << "\n";
            amrex::Print() << "    Min Load: " << min_load << "\n";
        }
        
        // Determine if this is the best direction
        // You can change this criteria based on what you prioritize
        bool is_better = false;
        
        // Option 1: Prioritize efficiency
        if (current_eff > best_efficiency) {
            is_better = true;
        }
        // Option 2: If efficiency is close, prefer lower variance
        else if (std::abs(current_eff - best_efficiency) < 0.001 && 
                 variance < best_load_variance) {
            is_better = true;
        }
        
        if (is_better || best_result.empty()) {
            best_result = current_result;
            best_efficiency = current_eff;
            best_load_variance = variance;
            best_direction_name = direction_names[d];
        }
    }
    
    if (flag_verbose_mapper) {
        amrex::Print() << "\n=== BEST DIRECTION: " << best_direction_name << " ===\n";
        amrex::Print() << "Best Efficiency: " << best_efficiency << "\n";
        amrex::Print() << "Best Load Variance: " << best_load_variance << "\n";
    }
    
    if (best_eff) *best_eff = best_efficiency;
    return best_result;
}