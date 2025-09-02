#ifndef HILBERTSFC_H
#define HILBERTSFC_H

#include <AMReX_BoxArray.H>
#include <AMReX_INT.H>
#include <AMReX_Extension.H>
#include <AMReX_Array.H>
#include <AMReX_Vector.H>
#include <cstdint>

#include "HilbertSFCToken.H" 
#include "HilbertDirection.H"  // Include the enum

uint64_t coords_to_hilbert(uint32_t x, uint32_t y, uint32_t z, int order, 
                          HilbertDirection direction = HilbertDirection::DEFAULT);

void HilbertDistribute (const std::vector<HilbertSFCToken>& tokens,
                        const std::vector<amrex::Long>& wgts,
                        int nprocs,
                        amrex::Real volpercpu,
                        std::vector<std::vector<int>>& v,
                        bool flag_verbose_mapper = false);

std::vector<int>
HilbertProcessorMapDoIt(const amrex::BoxArray& boxes,
                       const std::vector<amrex::Long>& wgts,
                       int nprocs,
                       amrex::Real* eff,
                       int node_size = 0,
                       bool flag_verbose_mapper = false,
                       bool sort = false,
                       const std::vector<amrex::Long>& bytes = std::vector<amrex::Long>(),
                       HilbertDirection direction = HilbertDirection::DEFAULT);

std::vector<int>
HilbertProcessorMapDoItWithDirectionTesting(const amrex::BoxArray& boxes,
                                           const std::vector<amrex::Long>& wgts,
                                           int nprocs,
                                           amrex::Real* best_eff,
                                           int node_size = 0,
                                           bool flag_verbose_mapper = false,
                                           bool sort = false,
                                           const std::vector<amrex::Long>& bytes = std::vector<amrex::Long>());

#endif