AMREX_HOME ?= ../../../../amrex

DEBUG	= FALSE
DIM	= 3
COMP    = gnu
PRECISION = DOUBLE

USE_MPI   = FALSE
USE_OMP   = FALSE
USE_CUDA  = FALSE
USE_HIP   = FALSE

BL_NO_FORT = TRUE

EBASE = main

# GPU settings
CUDA_ARCH = 80
CUDA_MAX_THREADS = 256

# Add source directories
VPATH_LOCATIONS += ..
INCLUDE_LOCATIONS += ..

include $(AMREX_HOME)/Tools/GNUMake/Make.defs

include ./Make.package
include $(AMREX_HOME)/Src/Base/Make.package

include $(AMREX_HOME)/Tools/GNUMake/Make.rules 