#ifndef AMREX_KARMARKAR
#define AMREX_KARMARKAR

#include <AMReX_INT.H>
#include <AMReX_Vector.H>

// =============================================================

void kk (const std::vector<amrex::Long>&  wgts,
          int                              nprocs,
          std::vector< std::vector<int> >& result,
          amrex::Real&                     efficiency,
          bool                             flag_verbose_mapper = false);
// =============================================================
std::vector<int> KKDoIt (const std::vector<amrex::Long>& wgts,
              int                             nprocs,
              amrex::Real&                    efficiency,
              bool                            sort,  //wgts sorted or not
              bool                            flag_verbose_mapper = false,
              const std::vector<amrex::Long>& bytes = std::vector<amrex::Long>());
#endif