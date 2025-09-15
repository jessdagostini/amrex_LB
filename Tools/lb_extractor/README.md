# WarpX Load Balancing Extractor
Scripts to run WarpX simulations, collect load balancing data and run through experimental LB algortihms

## Description
This repository contains scripts to automatize the collection of Load Balancing metrics from WarpX runs. Through the `ReducedDiagnostics` options on WarpX (more detail can be found [here](https://warpx.readthedocs.io/en/latest/usage/parameters.html)), the `LBC` diagnostics output relevant information used by the Load Balancing algorithms implemented on AMReX. What the scripts here do is to run WarpX simulations enabling these capturing and provides an efficient and automatized way to dispatch runs and parse the LB data to be tested with different balancing algorithms. To make it easier to test different load balancing strategies apart from the complexity of editting it on WarpX full source code, `amrex_LB` was developed. It contains all necessary elements to run tests with different load balancing algorithms by using both generated data or data captured from WarpX.


## Dependencies
- amrex_LB - Steps to compile can be found [here](https://github.com/jessdagostini/amrex_LB/blob/98d44fdcd4f8d50b75ee3537cbef8c184a7690bf/README.md)
- WarpX - Steps to compile can be found in their [documentation](https://warpx.readthedocs.io/en/latest/install/hpc.html) (for specific NERSC/Perlmutter installation, click [here](https://warpx.readthedocs.io/en/latest/install/hpc/perlmutter.html))
- Python 3.x

## Running and collecting data
TBD