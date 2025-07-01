#include <AMReX_INT.H>
#include <AMReX_REAL.H>
#include <AMReX_BLProfiler.H>
#include <AMReX_Vector.H>
#include <AMReX_Print.H>
#include <AMReX_ParallelContext.H>
#include <AMReX_ParallelReduce.H>

#include "Knapsack.H"
#include "LeastUsed.H"
#include "KK.h"

#include <queue>
#include <vector>
#include <fstream>
#include <algorithm>


// represents a box with weight and ID
class Box {
    int  m_boxid;
    amrex::Long m_weight;

public:
    Box (int b, int w) : m_boxid(b), m_weight(w) {}

    amrex::Long weight () const { return m_weight; }
    amrex::Long weight (int b) const { return m_weight; }
    
    int  boxid ()  const { return m_boxid;  }
    // Sort the boxes in descending order by weight
    bool operator< (const Box& rhs) const {
        return weight() > rhs.weight();
    }
};

struct BoxSubset {
    amrex::Vector<Box> boxes;
    amrex::Long totalWeight;

    BoxSubset() : totalWeight(0) {} // Default constructor
    BoxSubset(Box b) : totalWeight(b.weight()) {
        boxes.push_back(b);
    }

    void addBox(Box b) {
        boxes.push_back(b);
        totalWeight += b.weight();
    }

    amrex::Long weight() const {
        return totalWeight;
    }

    int size() const {
        return boxes.size();
    }

    bool operator< (const BoxSubset& rhs) const
    {
        return totalWeight < rhs.totalWeight; // Sort by total weight in ascending order
    }
};


struct BoxTuple {
    amrex::Vector<BoxSubset> subsets;
    amrex::Long difference = 0;

    // Default constructor initializes an empty BoxTuple with a given number of subsets
    BoxTuple(int nproc) {
        subsets.resize(nproc); // Initialize with empty subsets
    }
    BoxTuple(BoxSubset bs, int nproc) : difference(bs.totalWeight) {
        subsets.resize(nproc); // Initialize with empty subsets
        subsets[0] = bs; // Initialize the first subset with the given box
    }

    void update_difference() {
        std::sort(subsets.begin(), subsets.end()); // Sort subsets by total weight
        difference = subsets.back().weight() - subsets.front().weight();
    }

    int size() const {
        return subsets.size();
    }

    bool operator< (const BoxTuple& rhs) const
    {
        return difference < rhs.difference;
    }    
};

// =======================================================================================

/* In:  Vector of wgts
        Number of bins (nprocs)
        nmax -- max number of boxes (broken?)
        do_full_knapsack -- do adjustment step.

   Out: Vector<Vector< >> 
        Efficiency of result: 
*/

void
kk (const std::vector<amrex::Long>&  wgts,
          int                              nprocs,
          std::vector< std::vector<int> >& result,
          amrex::Real&                     efficiency,
          bool                            flag_verbose_mapper)
{
    BL_PROFILE("kk()");

    //
    // Sort balls by size largest first.
    //
    result.resize(nprocs);
    
    std::priority_queue<BoxTuple> boxQueue;
    
    for (unsigned int i = 0, N = wgts.size(); i < N; ++i) {
        Box element(i, wgts[i]);
        BoxSubset bs(element);
        BoxTuple bt(bs, nprocs); // Initialize with one subset
        boxQueue.push(bt);
    }

    while(boxQueue.size() > 1) {
        BoxTuple A = boxQueue.top();
        boxQueue.pop();
        std::sort(A.subsets.begin(), A.subsets.end()); // Sort subsets by total weight
        BoxTuple B = boxQueue.top();
        std::sort(B.subsets.begin(), B.subsets.end()); // Sort subsets by total weight
        boxQueue.pop();

        // Print both A and B for debugging
        // if (flag_verbose_mapper) {
        //     amrex::Print() << "Combining BoxTuple A with difference: " << A.difference << "\n";
        //     for (const auto& subset : A.subsets) {
        //         amrex::Print() << "Subset total weight: " << subset.weight() << "\n";
        //         for (const auto& box : subset.boxes) {
        //             amrex::Print() << "  Box ID: " << box.boxid() << ", Weight: " << box.weight() << "\n";
        //         }
        //     }

        //     amrex::Print() << "Combining BoxTuple B with difference: " << B.difference << "\n";
        //     for (const auto& subset : B.subsets) {
        //         amrex::Print() << "Subset total weight: " << subset.weight() << "\n";
        //         for (const auto& box : subset.boxes) {
        //             amrex::Print() << "  Box ID: " << box.boxid() << ", Weight: " << box.weight() << "\n";
        //         }
        //     }

        //     amrex::Print() << "\n";
        // }

        BoxTuple combined(nprocs); // Create a new BoxTuple for the combination
        int A_size = A.subsets.size();
        int B_size = B.subsets.size();

        for (int i = 0, j = B_size - 1; i < A_size && j >= 0; ++i, --j) {
            // Combine the lightest box from A with the heaviest box from B
            for (auto & box : A.subsets[i].boxes) {
                combined.subsets[i].addBox(box);
            }

            for (auto & box : B.subsets[j].boxes) {
                combined.subsets[i].addBox(box);
            }
        }

        combined.update_difference();

        // Debug combined BoxTuple
        // if (flag_verbose_mapper) {
        //     amrex::Print() << "Combined BoxTuple with difference: " << combined.difference << "\n";
        //     for (const auto& subset : combined.subsets) {
        //         amrex::Print() << "Subset total weight: " << subset.weight() << "\n";
        //         for (const auto& box : subset.boxes) {
        //             amrex::Print() << "  Box ID: " << box.boxid() << ", Weight: " << box.weight() << "\n";
        //         }
        //     }
        //     amrex::Print() << "\n";
        // }

        boxQueue.push(combined);
    }

    BoxTuple finalTuple = boxQueue.top();
    boxQueue.pop();
    
    // if (flag_verbose_mapper) {
    //     amrex::Print() << "Final BoxTuple with difference: " << finalTuple.difference << "\n";
    //     for (const auto& subset : finalTuple.subsets) {
    //         amrex::Print() << "Subset total weight: " << subset.weight() << "\n";
    //         for (const auto& box : subset.boxes) {
    //             amrex::Print() << "  Box ID: " << box.boxid() << ", Weight: " << box.weight() << "\n";
    //         }
    //     }
    // }

    amrex::Real bucket_weight = 0;
    amrex::Real max_weight = 0;

    // Map the boxes to the result vector
    for (int i = 0, N = finalTuple.size(); i < N; ++i) {
        const BoxSubset& bs = finalTuple.subsets[i];
        bucket_weight += bs.weight();
        max_weight = (max_weight < bs.weight()) ? bs.weight() : max_weight;

        result[i].reserve(bs.size());
        for (auto const& b : bs.boxes)
        {
            result[i].push_back(b.boxid());
        }
    }

    efficiency = bucket_weight/(nprocs*max_weight);
}


std::vector<int>
KKDoIt (const std::vector<amrex::Long>& wgts,// length of vector is the number of boxes
              int                             nprocs,// number of buckets
              amrex::Real&                    efficiency,//output
              bool                            sort,  //wgts sorted or not
              bool                            flag_verbose_mapper,//output distributed maps
              const std::vector<amrex::Long>& bytes)
{

    if (flag_verbose_mapper) {
        amrex::Print() << "DM: Karmarkar-Karp called..." << std::endl;
    }

    BL_PROFILE("KKDoIt()");

    //nprocs = amrex::ParallelContext::NProcsSub();

    // If team is not use, we are going to treat it as a special case in which
    // the number of teams is nprocs and the number of workers is 1.

    int nteams = nprocs;
    int nworkers = 1;
    /*
    #if defined(BL_USE_TEAM)
        nteams = ParallelDescriptor::NTeams();
        nworkers = ParallelDescriptor::TeamSize();
    #endif
    */
    std::vector< std::vector<int> > vec;

    efficiency = 0;

    kk(wgts,nteams,vec,efficiency,flag_verbose_mapper);

    if (flag_verbose_mapper) {
        for (int i = 0, ni = vec.size(); i < ni; ++i) {
            // amrex::Print() << "  Bucket " << i << " contains boxes:" << std::endl << "    ";
            for (int j = 0, nj = vec[i].size(); j < nj; ++j) {
                // amrex::Print() << vec[i][j] << " ";
            }
            // amrex::Print() << std::endl;
        }
    }

    BL_ASSERT(static_cast<int>(vec.size()) == nteams);

    std::vector<LIpair> LIpairV;

    LIpairV.reserve(nteams);

    for (int i = 0; i < nteams; ++i) {
        amrex::Long wgt = 0;
        for (std::vector<int>::const_iterator lit = vec[i].begin(), End = vec[i].end(); lit != End; ++lit) {
            wgt += wgts[*lit];
        }

        LIpairV.push_back(LIpair(wgt,i));
    }

    if (sort) {Sort(LIpairV, true);}

    if (flag_verbose_mapper) {
        for (const auto &p : LIpairV) {
            amrex::Print() << "  Bucket " << p.second << " total weight: " << p.first << std::endl;
        }
    }

    amrex::Vector<int> ord;// ordering of the buckets 
    amrex::Vector<amrex::Vector<int> > wrkerord; //mapping of boxes to boxes for 
                                                //the algorithm

    if (nteams == nprocs) {
        if (sort) {
            LeastUsedCPUs(nprocs,bytes,ord,flag_verbose_mapper);
        } else {
            ord.resize(nprocs);
            std::iota(ord.begin(), ord.end(), 0);
        }
    } else {
        if (sort) {
//            LeastUsedTeams(ord,wrkerord,nteams,nworkers);
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

    for (int i = 0; i < nteams; ++i)
    {
        const int idx = LIpairV[i].second;
        const int tid = ord[i];

        const std::vector<int>& vi = vec[idx];
        const int N = vi.size();

        // if (flag_verbose_mapper) {
        //     amrex::Print() << "  Mapping bucket " << idx << " to rank " << tid << std::endl;
        // }

        if (nteams == nprocs) {
            for (int j = 0; j < N; ++j)
            {
               result[vi[j]] = tid;
            }
        } else {
#ifdef BL_USE_TEAM
            int leadrank = tid * nworkers;
            for (int w = 0; w < nworkers; ++w)
            {
                ParallelDescriptor::team_for(0, N, w, [&] (int j) {
                        result[vi[j]] = leadrank + wrkerord[i][w];
                    });
            }
#endif
        }
    }

    if (flag_verbose_mapper)
    {
        amrex::Print() << "Only Karmarkar-Karp efficiency: " << efficiency << '\n';
        amrex::Print() << "test......: " << '\n';
    }

    // // Output the distribution map to a CSV file
    // std::ofstream outfile("distribution_map_knapsack.csv");
    // outfile << "BoxID,Processor,Weight\n";
    // for (size_t i = 0; i < result.size(); ++i) {
    //     outfile << i << "," << result[i] << "," << wgts[i] << "\n";
    // }
    // outfile.close();


    return result;

    // amrex::Print() << "test......: " << '\n';
// Output the distribution map to a CSV file
//     std::ofstream outfile("distribution_map_knapsack.csv");
//     for (const auto& elem : result) {
//         outfile << elem << "\n";
//     }
//     outfile.close();

//     return result;
}