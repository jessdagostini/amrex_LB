#include <AMReX_INT.H>
#include <AMReX_REAL.H>
#include <AMReX_BLProfiler.H>
#include <AMReX_Vector.H>
#include <AMReX_Print.H>
#include <AMReX_ParallelContext.H>
#include <AMReX_ParallelReduce.H>

#include "Knapsack.H"
#include "LeastUsed.H"

#include <queue>
#include <vector>
#include <fstream>



// Sort function sorts the vector LIpair, reverse flag determines 
// if the sorting to be done in descending order.

void
Sort (std::vector<LIpair>& vec,
      bool                 reverse)
{
    if (vec.size() > 1)
    {
        if (reverse) {
            std::stable_sort(vec.begin(), vec.end(), LIpairGT());
        }
        else {
            std::stable_sort(vec.begin(), vec.end(), LIpairLT());
        }
    }
}


// represents a box with weight and ID

class WeightedBox
{
    int  m_boxid;
    amrex::Long m_weight;
    std::size_t m_memory;
public:
    WeightedBox (int b, int w) : m_boxid(b), m_weight(w) {}
    WeightedBox (int b, int w, std::size_t m) : m_boxid(b), m_weight(w), m_memory(m) {}
    amrex::Long weight () const { return m_weight; }
    amrex::Long weight (int b) const { return m_weight; }
    int  boxid ()  const { return m_boxid;  }
    std::size_t memory () const { return m_memory; }
    // Sort the boxes in descending order by weight and memory
    bool operator< (const WeightedBox& rhs) const
    {
        if (weight() != rhs.weight()) {
            return weight() > rhs.weight();
        }
        return memory() > rhs.memory();
    }
};

// class to add, erase and access boxes 


class WeightedBoxList
{
    amrex::Vector<WeightedBox>* m_lb;
    amrex::Long                 m_weight;
    std::size_t                 m_memory;
public:
    WeightedBoxList (amrex::Long w) : m_lb(nullptr), m_weight(w), m_memory(0) {}
    WeightedBoxList (amrex::Long w, std::size_t m) : m_lb(nullptr), m_weight(w), m_memory(m) {}
    WeightedBoxList (amrex::Vector<WeightedBox>* lb) : m_lb(lb), m_weight(0), m_memory(0) {}
    amrex::Long weight () const
    {
        return m_weight;
    }
    std::size_t memory () const
    {
        return m_memory;
    }
    void addWeight (amrex::Long dw) { m_weight += dw; }
    void addMemory (std::size_t dm) { m_memory += dm; }
    void erase (amrex::Vector<WeightedBox>::iterator& it)
    {
        m_weight -= it->weight();
        m_memory -= it->memory();
        m_lb->erase(it);
    }
    void push_back (const WeightedBox& bx)
    {
        m_weight += bx.weight();
        m_memory += bx.memory();
        m_lb->push_back(bx);
    }
    int size () const { return m_lb->size(); }
    amrex::Vector<WeightedBox>::const_iterator begin () const { return m_lb->begin(); }
    amrex::Vector<WeightedBox>::iterator begin ()             { return m_lb->begin(); }
    amrex::Vector<WeightedBox>::const_iterator end () const   { return m_lb->end();   }
    amrex::Vector<WeightedBox>::iterator end ()               { return m_lb->end();   }

    bool operator< (const WeightedBoxList& rhs) const
    {
        if (weight() != rhs.weight()) {
            return weight() > rhs.weight();
        }
        return memory() > rhs.memory();
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
knapsack (const std::vector<amrex::Long>&  wgts,
          int                              nprocs,
          std::vector< std::vector<int> >& result,
          amrex::Real&                     efficiency,
          bool                             do_full_knapsack,
          int                              nmax,
          const amrex::Real&               max_efficiency)
{
    BL_PROFILE("knapsack()");

    //
    // Sort balls by size largest first.
    //
    result.resize(nprocs);
    
    amrex::Vector<WeightedBox> lb;
    lb.reserve(wgts.size());
    for (unsigned int i = 0, N = wgts.size(); i < N; ++i)
    {
        lb.push_back(WeightedBox(i, wgts[i]));
    }
    std::sort(lb.begin(), lb.end());
    //
    // For each ball, starting with heaviest, assign ball to the lightest bin.
    // (This is the seeding of each bin. Several randomized attempts + this?)
    //
    std::priority_queue<WeightedBoxList> wblq;
    amrex::Vector<std::unique_ptr<amrex::Vector<WeightedBox> > > raii_vwb(nprocs);
    for (int i  = 0; i < nprocs; ++i)
    {
        raii_vwb[i] = std::make_unique<amrex::Vector<WeightedBox> >();
        wblq.push(WeightedBoxList(raii_vwb[i].get()));
    }
    amrex::Vector<WeightedBoxList> wblv;
    wblv.reserve(nprocs);
    for (unsigned int i = 0, N = wgts.size(); i < N; ++i)
    {
        if (!wblq.empty()) {
            WeightedBoxList wbl = wblq.top();
            wblq.pop();
            wbl.push_back(lb[i]);
            if (wbl.size() < nmax) {
                wblq.push(wbl);
            } else {
                wblv.push_back(wbl);
            }
        } else {
            int ip = static_cast<int>(i) % nprocs;
            wblv[ip].push_back(lb[i]);
        }
    }

    amrex::Real max_weight = 0;
    amrex::Real bucket_weight = 0;
    for (auto const& wbl : wblv)
    {
        amrex::Real wgt = wbl.weight();
        bucket_weight += wgt;
        max_weight = std::max(wgt, max_weight);
    }

    while (!wblq.empty())
    {
        WeightedBoxList wbl = wblq.top();
        wblq.pop();
        if (wbl.size() > 0) {
            amrex::Real wgt = wbl.weight();
            bucket_weight += wgt;
            max_weight = std::max(wgt, max_weight);
            wblv.push_back(wbl);
        }
    }

    efficiency = bucket_weight/(nprocs*max_weight);

    std::sort(wblv.begin(), wblv.end());

    if (efficiency < max_efficiency && do_full_knapsack
        && wblv.size() > 1 && wblv.begin()->size() > 1)
    {
        BL_PROFILE_VAR("knapsack()swap", swap);

        top: ;

        if (efficiency < max_efficiency && wblv.begin()->size() > 1)
        {
            auto bl_top = wblv.begin();
            auto bl_bottom = wblv.end()-1;
            amrex::Long w_top = bl_top->weight();
            amrex::Long w_bottom = bl_bottom->weight();
            for (auto ball_1 = bl_top->begin(); ball_1 != bl_top->end(); ++ball_1)
            {
                for (auto ball_2 = bl_bottom->begin(); ball_2 != bl_bottom->end(); ++ball_2)
                {
                    // should we swap ball 1 and ball 2?
                    amrex::Long dw = ball_1->weight() - ball_2->weight();
                    amrex::Long w_top_new    = w_top    - dw;
                    amrex::Long w_bottom_new = w_bottom + dw;
                    if (w_top_new < w_top && w_bottom_new < w_top)
                    {
                        std::swap(*ball_1, *ball_2);
                        bl_top->addWeight(-dw);
                        bl_bottom->addWeight(dw);

                        if (bl_top+1 == bl_bottom)  // they are next to each other
                        {
                            if (*bl_bottom < *bl_top) {
                                std::swap(*bl_top, *bl_bottom);
                            }
                        }
                        else
                        {
                            // bubble up
                            auto it = std::lower_bound(bl_top+1, bl_bottom, *bl_bottom);
                            std::rotate(it, bl_bottom, bl_bottom+1);

                            // sink down
                            it = std::lower_bound(bl_top+1, bl_bottom+1, *bl_top);
                            std::rotate(bl_top, bl_top+1, it);
                        }

                        max_weight = bl_top->weight();
                        efficiency = bucket_weight / (nprocs*max_weight);
                        goto top;
                    }
                }
            }
        }

        BL_ASSERT(std::is_sorted(wblv.begin(), wblv.end()));
    }

    for (int i = 0, N = wblv.size(); i < N; ++i)
    {
        const WeightedBoxList& wbl = wblv[i];

        result[i].reserve(wbl.size());
        for (auto const& wb : wbl)
        {
            result[i].push_back(wb.boxid());
        }
    }
}

void
knapsack (const std::vector<amrex::Long>&  wgts,
          int                              nprocs,
          std::vector< std::vector<int> >& result,
          amrex::Real&                     efficiency,
          const std::vector<std::size_t>& memory,
          bool                             do_full_knapsack,
          std::size_t                      nmax,
          const amrex::Real&               max_efficiency)
{
    BL_PROFILE("knapsack()");

    //
    // Sort balls by size largest first.
    //
    result.resize(nprocs);
    
    amrex::Vector<WeightedBox> lb;
    lb.reserve(wgts.size());
    for (unsigned int i = 0, N = wgts.size(); i < N; ++i)
    {
        lb.push_back(WeightedBox(i, wgts[i], memory[i]));
    }
    std::sort(lb.begin(), lb.end());

    // Print lb for debugging
    for (const auto& box : lb) {
        amrex::Print() << "Box ID: " << box.boxid() << ", Weight: " << box.weight() << ", Memory: " << box.memory() << "\n";
    }
    
    std::priority_queue<WeightedBoxList> wblq;
    amrex::Vector<std::unique_ptr<amrex::Vector<WeightedBox> > > raii_vwb(nprocs);
    for (int i  = 0; i < nprocs; ++i)
    {
        raii_vwb[i] = std::make_unique<amrex::Vector<WeightedBox> >();
        wblq.push(WeightedBoxList(raii_vwb[i].get()));
    }
    amrex::Vector<WeightedBoxList> wblv;
    wblv.reserve(nprocs);
    for (unsigned int i = 0, N = wgts.size(); i < N; ++i)
    {
        if (!wblq.empty()) {
            WeightedBoxList wbl = wblq.top();
            wblq.pop();

            // Check if I still have memory space in this bin
            amrex::Print() << "Trying to add box " << lb[i].boxid() << " with memory " << lb[i].memory() << " to bin with current memory " << wbl.memory() << " and max memory " << nmax << "\n";
            if ((wbl.memory() + lb[i].memory()) < nmax) {
                wbl.push_back(lb[i]);
                wblq.push(wbl);
            } else {
                // This is a full bin, so I can't add any more boxes to it and need to remove it from the available bins
                wblv.push_back(wbl);
            }
        } else {
            // If I run out of bins, this means I run out of memory space in all bins
            // Thus, I should return an error because I don't have space to put all boxes
            amrex::Abort("Knapsack Error: Not enough memory space in bins to allocate all boxes");
            return;
        }
    }

    amrex::Real max_weight = 0;
    amrex::Real bucket_weight = 0;
    for (auto const& wbl : wblv)
    {
        amrex::Real wgt = wbl.weight();
        bucket_weight += wgt;
        max_weight = std::max(wgt, max_weight);
    }

    while (!wblq.empty())
    {
        WeightedBoxList wbl = wblq.top();
        wblq.pop();
        if (wbl.size() > 0) {
            amrex::Real wgt = wbl.weight();
            bucket_weight += wgt;
            max_weight = std::max(wgt, max_weight);
            wblv.push_back(wbl);
        }
    }

    efficiency = bucket_weight/(nprocs*max_weight);

    std::sort(wblv.begin(), wblv.end());

    if (efficiency < max_efficiency && do_full_knapsack
        && wblv.size() > 1 && wblv.begin()->size() > 1)
    {
        BL_PROFILE_VAR("knapsack()swap", swap);
top: ;

        if (efficiency < max_efficiency && wblv.begin()->size() > 1)
        {
            auto bl_top = wblv.begin();
            auto bl_bottom = wblv.end()-1;
            amrex::Long w_top = bl_top->weight();
            amrex::Long w_bottom = bl_bottom->weight();
            std::size_t mem_top = bl_top->memory();
            std::size_t mem_bottom = bl_bottom->memory();
            for (auto ball_1 = bl_top->begin(); ball_1 != bl_top->end(); ++ball_1)
            {
                for (auto ball_2 = bl_bottom->begin(); ball_2 != bl_bottom->end(); ++ball_2)
                {
                    // should we swap ball 1 and ball 2?
                    amrex::Long dw = ball_1->weight() - ball_2->weight();
                    size_t dm = ball_1->memory() - ball_2->memory();
                    amrex::Long w_top_new    = w_top    - dw;
                    amrex::Long w_bottom_new = w_bottom + dw;
                    std::size_t mem_top_new    = mem_top    - dm;
                    std::size_t mem_bottom_new = mem_bottom + dm;
                    if (w_top_new < w_top && w_bottom_new < w_top && 
                        mem_top_new < nmax && mem_bottom_new < nmax)
                    {
                        std::swap(*ball_1, *ball_2);
                        bl_top->addWeight(-dw);
                        bl_top->addMemory(-dm);
                        bl_bottom->addWeight(dw);
                        bl_bottom->addMemory(dm);

                        if (bl_top+1 == bl_bottom)  // they are next to each other
                        {
                            if (*bl_bottom < *bl_top) {
                                std::swap(*bl_top, *bl_bottom);
                            }
                        }
                        else
                        {
                            // bubble up
                            auto it = std::lower_bound(bl_top+1, bl_bottom, *bl_bottom);
                            std::rotate(it, bl_bottom, bl_bottom+1);

                            // sink down
                            it = std::lower_bound(bl_top+1, bl_bottom+1, *bl_top);
                            std::rotate(bl_top, bl_top+1, it);
                        }

                        max_weight = bl_top->weight();
                        efficiency = bucket_weight / (nprocs*max_weight);
                        goto top;
                    }
                }
            }
        }

        BL_ASSERT(std::is_sorted(wblv.begin(), wblv.end()));
    }

    for (int i = 0, N = wblv.size(); i < N; ++i)
    {
        const WeightedBoxList& wbl = wblv[i];

        result[i].reserve(wbl.size());
        for (auto const& wb : wbl)
        {
            result[i].push_back(wb.boxid());
        }
    }
}


std::vector<int>
KnapSackDoIt (const std::vector<amrex::Long>& wgts,// length of vector is the number of boxes
              int                             nprocs,// number of buckets
              amrex::Real&                    efficiency,//output
              bool                            do_full_knapsack,
              int                             nmax,//limit max number of boxes in the buckets, 
              bool                            flag_verbose_mapper,//output distributed maps
              bool                            sort,  //wgts sorted or not
              const std::vector<amrex::Long>& bytes)
{
    sort = false;

    if (flag_verbose_mapper) {
        amrex::Print() << "DM: KnapSackDoIt called..." << std::endl;
    }

    BL_PROFILE("KnapSackDoIt()");

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

    knapsack(wgts,nteams,vec,efficiency,do_full_knapsack,nmax);

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

    for (int i = 0; i < nteams; ++i)
    {
        amrex::Long wgt = 0;
        for (std::vector<int>::const_iterator lit = vec[i].begin(), End = vec[i].end();
             lit != End; ++lit)
        {
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

    for (const auto &p : LIpairV) {
        fprintf(stderr, "%s, %s, %0.10f, %d\n", 
                algorithms[MetricUtilsAlgorithms::KNAPSACK],
                metrics[MetricUtilsMetrics::WEIGHT],
                p.first, p.second);
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
        amrex::Print() << "Only KNAPSACK  efficiency: " << efficiency << '\n';
        amrex::Print() << "test......: " << '\n';
    }

    fprintf(stderr, "%s, %s, %0.10f\n", 
            algorithms[MetricUtilsAlgorithms::KNAPSACK],
            metrics[MetricUtilsMetrics::EFFICIENCY],
            efficiency);

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

std::vector<int>
KnapSackDoItMem (const std::vector<amrex::Long>& wgts,// length of vector is the number of boxes
                const std::vector<std::size_t>& memory,
                int                             nprocs,// number of buckets
                amrex::Real&                    efficiency,//output
                bool                            do_full_knapsack,
                std::size_t                     maxmem,//limit max memory in the buckets,
                bool                            flag_verbose_mapper,//output distributed maps
                bool                            sort,  //wgts sorted or not
                const std::vector<amrex::Long>& bytes)
{
    sort = false;

    if (flag_verbose_mapper) {
        amrex::Print() << "DM: KnapSackDoItMem called..." << std::endl;
    }

    BL_PROFILE("KnapSackDoItMem()");

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

    knapsack(wgts,nteams,vec,efficiency,memory,do_full_knapsack,maxmem);

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

    for (int i = 0; i < nteams; ++i)
    {
        amrex::Long wgt = 0;
        for (std::vector<int>::const_iterator lit = vec[i].begin(), End = vec[i].end();
             lit != End; ++lit)
        {
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

    for (const auto &p : LIpairV) {
        fprintf(stderr, "%s, %s, %0.10f, %d\n", 
                algorithms[MetricUtilsAlgorithms::KNAPSACK],
                metrics[MetricUtilsMetrics::WEIGHT],
                p.first, p.second);
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
        amrex::Print() << "Only KNAPSACK  efficiency: " << efficiency << '\n';
        amrex::Print() << "test......: " << '\n';
    }

    fprintf(stderr, "%s, %s, %0.10f\n", 
            algorithms[MetricUtilsAlgorithms::KNAPSACK],
            metrics[MetricUtilsMetrics::EFFICIENCY],
            efficiency);

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


#if 0

void
DistributionMapping::KnapSackProcessorMap (const std::vector<Long>& wgts,
                                           int                      nprocs,
                                           Real*                    efficiency,
                                           bool                     do_full_knapsack,
                                           int                      nmax,
                                           bool                     sort)
{
    BL_ASSERT(wgts.size() > 0);

    m_ref->clear();
    m_ref->m_pmap.resize(wgts.size());

    if (static_cast<int>(wgts.size()) <= nprocs || nprocs < 2)
    {
        RoundRobinProcessorMap(wgts.size(),nprocs, sort);

        if (efficiency) *efficiency = 1;
    }
    else
    {
        Real eff = 0;
        KnapSackDoIt(wgts, nprocs, eff, do_full_knapsack, nmax, sort);
        if (efficiency) *efficiency = eff;
    }
}

DistributionMapping
DistributionMapping::makeKnapSack (const Vector<Real>& rcost, int nmax)
{
    BL_PROFILE("makeKnapSack");

    DistributionMapping r;

    Vector<Long> cost(rcost.size());

    Real wmax = *std::max_element(rcost.begin(), rcost.end());
    Real scale = (wmax == 0) ? 1.e9_rt : 1.e9_rt/wmax;

    for (int i = 0; i < rcost.size(); ++i) {
        cost[i] = Long(rcost[i]*scale) + 1L;
    }

    int nprocs = ParallelContext::NProcsSub();
    Real eff;

    r.KnapSackProcessorMap(cost, nprocs, &eff, true, nmax);

    return r;
}

#endif
