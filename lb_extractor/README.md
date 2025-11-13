# WarpX Load Balancing Extractor
Scripts to run WarpX simulations, collect load balancing data and run through experimental LB algortihms

## Description
This folder contains scripts to automatize the collection of Load Balancing metrics from WarpX runs. Through the `ReducedDiagnostics` options on WarpX (more detail can be found [here](https://warpx.readthedocs.io/en/latest/usage/parameters.html)), the `LBC` diagnostics output relevant information used by the Load Balancing algorithms implemented on AMReX. What the scripts here do is to run WarpX simulations enabling these capturing and provides an efficient and automatized way to dispatch runs and parse the LB data to be tested with different balancing algorithms. To make it easier to test different load balancing strategies apart from the complexity of editting it on WarpX full source code, `amrex_LB` was developed. It contains all necessary elements to run tests with different load balancing algorithms by using both generated data or data captured from WarpX.


## Dependencies
- amrex_LB - Steps to compile can be found [here](https://github.com/jessdagostini/amrex_LB/blob/98d44fdcd4f8d50b75ee3537cbef8c184a7690bf/README.md)
- WarpX - Steps to compile can be found in their [documentation](https://warpx.readthedocs.io/en/latest/install/hpc.html) (for specific NERSC/Perlmutter installation, click [here](https://warpx.readthedocs.io/en/latest/install/hpc/perlmutter.html))
- Python 3.x

## Running and collecting data
All description here is based on running the `laser_ion_plasma` input set available [here](). Sample files are available in the directories in the repository.

1. Generate a real execution to collect load balancing data from WarpX
Using the `generate_and_run_sbatch.py` script, the user can generate and automatically launch a `sbatch` job to run and collect data from WarpX. Note that, for this step, is assumed that WarpX and its dependencies is already compiled and ready to use. We are also basing our script on NERSC Slurm headers.

    Within the python script it is possible to change some variables that affect the load balancing performance from the test case, which are the following
    ```
    NERSC_PROJECT = 'project_name_here_g' #_g is needed to use NERSC GPUs
    MAX_STEPS = 1000
    N_CELLS = [7488, 14720]
    BLOCKING_FACTOR = [32, 32]
    MAX_GRID_SIZE = [512, 512]
    LOAD_BALANCE_INTERVALS = 100
    LOAD_BALANCE_COSTS_UPDATE = "timers" #or weights
    LBC_OUTPUT_INTERVAL = 100
    LBC_OUTPUT_TYPE = "LoadBalanceCosts"
    NODES = 6
    RATIO = 1.1
    ```
    The above are default values from our script, that can be updated as needed. All these variables will overwrite same definitions from the WarpX input file.

    The following command line will run the script and launche the job
    `python3 generate_and_run_sbatch.py <path/to/input_file>`

    At the end of the execution, the script will also parse the LBC data calling the `parse_lb_data.py` script available in this folder.

2. The data parsed from the batch job will be moved to the `lb_inputs` folder. An example of it is available for demonstration.

3. Using the parsed data, the user can then run the Load Balancer algorithms using the real data inputs from WarpX. To ease the execution, the `run_lb.py` script will read the `.json` file created onver the real execution and will parse all information needed for the LB "simulator" to execute. Results from this script will be stored at `lb_results` folder. Some examples of results are already available.

4. With the collected data, the user can perform data analysis using any tool of preference.