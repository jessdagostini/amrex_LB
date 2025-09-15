#include "HilbertSFCToken.H"
#include "HilbertSFC.H"
#include <AMReX_IntVect.H>
#include <algorithm>
#include <HilbertDirection.H>

bool HilbertSFCToken::Compare::operator()(const HilbertSFCToken& lhs,
                                        const HilbertSFCToken& rhs) const {
    return lhs.m_hilbert_key < rhs.m_hilbert_key;
}

HilbertSFCToken makeHilbertSFCToken(int box_index, amrex::IntVect const& iv, 
                                   HilbertDirection direction) {
    HilbertSFCToken token;
    token.m_box = box_index;

    // Use order 8 for better performance in large simulations
    constexpr int order = 8;  // 256^3 grid
    constexpr int imin = -(1 << (order-1));  // -128
    constexpr int imax = (1 << (order-1)) - 1;  // 127
    
    // Clamp coordinates to valid range and shift to positive
    uint32_t x = std::max(imin, std::min(imax, iv[0])) - imin;
    uint32_t y = std::max(imin, std::min(imax, iv[1])) - imin;
    uint32_t z = std::max(imin, std::min(imax, iv[2])) - imin;

    token.m_hilbert_key = coords_to_hilbert(x, y, z, order, direction);
    return token;
}