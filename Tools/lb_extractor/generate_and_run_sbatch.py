#!/usr/bin/env python

import os
import sys
import shutil
import subprocess
import re
import time
import json

SCRATCH = os.environ['SCRATCH']
HOME = os.environ['HOME']
MAX_STEPS = 5000
N_CELLS = [7488, 14720]  # [nx, nz]
BLOCKING_FACTOR = [32, 32]  # [bx, bz]
MAX_GRID_SIZE = [512, 512]  # [gx, gz]
LOAD_BALANCE_INTERVALS = 100
LOAD_BALANCE_COSTS_UPDATE = "timers"
LBC_OUTPUT_INTERVAL = 100
LBC_OUTPUT_TYPE = "LoadBalanceCosts"
NODES = 6
RATIO = 1.1
# Calculate boxes per rank assuming 4 ranks per node
total_boxes = (N_CELLS[0] // MAX_GRID_SIZE[0]) * (N_CELLS[1] // MAX_GRID_SIZE[1])
box_per_rank = total_boxes // (NODES * 4)
JOB_NAME = "laser_ion_%d_boxes_%d_ranks_timers" % (box_per_rank, (NODES * 4))


INPUT_NAME = sys.argv[1]
if len(sys.argv) < 2:
    print("Usage: generate-sbatch.py <input_file>")
    print("Example: generate-sbatch.py warpx_input")
    print("The input file is expected to be in the /warpx-inputs folder.")
    exit(-1)

# Create the folder with the job files
job_folder = os.path.join(SCRATCH, JOB_NAME + "_f")
if not os.path.exists(job_folder):
    os.makedirs(job_folder)

print(f"Job folder created at: {job_folder}")

# Copy executable to the job folder
executable = os.path.join(HOME, "src", "warpx", "build_pm_gpu", "bin", "warpx.2d")
try:
    os.system("cp %s %s" % (executable, job_folder))
    print(f"Executable copied to: {os.path.join(job_folder, 'warpx.2d')}")
except Exception as e:
    print(f"Error copying executable: {e}")

# Copy the input file to the job folder
try:
    os.system("cp %s %s" % (os.path.join("./warpx_inputs", INPUT_NAME), job_folder))
    print(f"Input file copied to: {os.path.join(job_folder, INPUT_NAME)}")
except Exception as e:
    print(f"Error copying input file: {e}")

# Create the sbatch job file
sbatch_header = f"""#!/bin/bash -l

#SBATCH -t 00:45:00
#SBATCH -N {NODES}
#SBATCH -J {JOB_NAME}
#    note: <proj> must end on _g
#SBATCH -A nintern_g
#SBATCH -q regular
# A100 40GB (most nodes)
#SBATCH -C gpu
# A100 80GB (256 nodes)
#SBATCH --exclusive
# ideally single:1, but NERSC cgroups issue
#SBATCH --gpu-bind=none
#SBATCH --ntasks-per-node=4
#SBATCH --gpus-per-node=4
#SBATCH -o {JOB_NAME}.o%j
#SBATCH -e {JOB_NAME}.e%j
"""

module_loads = """
module load gpu
module load PrgEnv-gnu
module load craype
module load craype-x86-milan
module load craype-accel-nvidia80
module load cudatoolkit
module load cmake/3.30.2
module load cray-hdf5-parallel/1.12.2.9
"""

main_script = """
EXE=./warpx.2d
INPUTS=%s

# pin to closest NIC to GPU
export MPICH_OFI_NIC_POLICY=GPU

# threads for OpenMP and threaded compressors per MPI rank
#   note: 16 avoids hyperthreading (32 virtual cores, 16 physical)
export OMP_NUM_THREADS=16

# GPU-aware MPI optimizations
GPU_AWARE_MPI="amrex.use_gpu_aware_mpi=1"

# Change grid size, blocking factor, steps, and other parameters in the input file
MAX_STEPS="max_step = %d"
N_CELLS="amr.n_cell = %d %d"
BLOCKING_FACTOR="amr.blocking_factor = %d %d"
MAX_GRID_SIZE="amr.max_grid_size = %d %d"
INTERVALS="algo.load_balance_intervals = %d"
COSTS_UPDATE="algo.load_balance_costs_update = %s"
LBC_INTERVALS="LBC.intervals = %d"
TYPE="LBC.type = %s"
RATIO="algo.load_balance_efficiency_ratio = %f"


# CUDA visible devices are ordered inverse to local task IDs
#   Reference: nvidia-smi topo -m
srun --cpu-bind=cores bash -c "export CUDA_VISIBLE_DEVICES=\$((3-SLURM_LOCALID));
    ${EXE} ${INPUTS} ${GPU_AWARE_MPI} ${MAX_STEPS} ${N_CELLS} ${BLOCKING_FACTOR} ${MAX_GRID_SIZE} ${INTERVALS} ${COSTS_UPDATE}  ${LBC_INTERVALS} ${TYPE} ${RATIO}" > output.txt


""" % (INPUT_NAME, MAX_STEPS, N_CELLS[0], N_CELLS[1], BLOCKING_FACTOR[0], BLOCKING_FACTOR[1], MAX_GRID_SIZE[0], MAX_GRID_SIZE[1], LOAD_BALANCE_INTERVALS, LOAD_BALANCE_COSTS_UPDATE, LBC_OUTPUT_INTERVAL, LBC_OUTPUT_TYPE, RATIO)

LBC_path = os.path.join(job_folder, 'diags', 'reducedfiles', 'LBC.txt')
LBC_parsed = os.path.join(job_folder, JOB_NAME + '_lb_data.txt')
    
parse_lb_data = """
# Parse load balancing data from the output file and save it to a new file.
module load python
python3 %s %s %d %s
""" % (os.path.join(HOME, 'lb_extractor', 'parse_lb_data.py'), LBC_path, MAX_STEPS, LBC_parsed)

# Move data back to home folder
move_data = """
# Move the load balancing data file to the home directory
mv %s %s
""" % (LBC_parsed, os.path.join(HOME, "lb_extractor", "lb_inputs", JOB_NAME + '_lb_data.txt'))

job_file = os.path.join(job_folder, "run_warpx.sbatch")
print(f"Creating job file: {job_file}")
with open(job_file, "w") as fh:
    fh.writelines(sbatch_header)
    fh.writelines(module_loads)
    fh.writelines(main_script)
    fh.writelines(parse_lb_data)
    fh.writelines(move_data)

# Create a json file with the parameters
json_file = os.path.join(HOME, "lb_extractor", "lb_inputs", JOB_NAME + ".json")
json_content = {
    "input_name": INPUT_NAME,
    "max_steps": MAX_STEPS,
    "n_cells": N_CELLS,
    "blocking_factor": BLOCKING_FACTOR,
    "max_grid_size": MAX_GRID_SIZE,
    "load_balance_intervals": LOAD_BALANCE_INTERVALS,
    "load_balance_costs_update": LOAD_BALANCE_COSTS_UPDATE,
    "lbc_output_interval": LBC_OUTPUT_INTERVAL,
    "lbc_output_type": LBC_OUTPUT_TYPE,
    "nodes": NODES,
    "job_name": JOB_NAME,
    "ratio": RATIO,
    "file_path": os.path.join(HOME, "lb_extractor", "lb_inputs", JOB_NAME + '_lb_data.txt')
}

with open(json_file, "w") as json_fh:
    json.dump(json_content, json_fh, indent=4)

# Enter the job folder to run the sbatch command
os.chdir(job_folder)

# Submit the job to Slurm and capture its output to get the job ID
print(f"Submitting job with sbatch: {job_file}")
job_id = None
try:
    # Submit the job and capture its output, which contains the job ID
    submit_result = subprocess.run(['sbatch', job_file], check=True, capture_output=True, text=True)

    # Extract the job ID from the output string "Submitted batch job 12345"
    match = re.search(r'Submitted batch job (\d+)', submit_result.stdout)
    if not match:
        print("Error: Could not parse Job ID from sbatch output.")
        print(f"Output was: {submit_result.stdout}")
        exit(-1)
        
    job_id = match.group(1)
    print(f"Job submitted successfully with ID: {job_id}")

except subprocess.CalledProcessError as e:
    print("Error submitting job to Slurm.")
    print(e.stderr)
    exit(-1)