#ifndef AMREX_HILBERTSFCTOKEN_H
#define AMREX_HILBERTSFCTOKEN_H

#include <cstdint>
#include <AMReX_IntVect.H>
#include "HilbertDirection.H"  // Include the enum

struct HilbertSFCToken {
    class Compare {
    public:
        bool operator () (const HilbertSFCToken& lhs,
                         const HilbertSFCToken& rhs) const;
    };
    int m_box;
    uint64_t m_hilbert_key;
};

HilbertSFCToken makeHilbertSFCToken(int box_index, amrex::IntVect const& iv, 
                                   HilbertDirection direction = HilbertDirection::DEFAULT);

#endif