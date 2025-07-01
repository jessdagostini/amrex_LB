#!/bin/bash
#SBATCH --qos=regular
#SBATCH --constraint=cpu
#SBATCH --time=01:30:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --account=nintern
#SBATCH --job-name=kk_scale_run
#SBATCH --mail-type=start,end,fail
#SBATCH --mail-user=jessicadagostini@lbl.gov
#SBATCH --output="%x_%A.out"
#SBATCH --error="%x_%A.err"

echo "Starting ${SLURM_JOB_NAME} on $(hostname) at $(date)"
# Load the necessary modules
module load PrgEnv-gnu

# Copy binary to $SCRATCH
echo "Copying binary to $SCRATCH"
cp /global/homes/j/jessd/amrex_LB/src/optimized_algorithms/main3d.gnu.x86-milan.TPROF.OMP.ex $SCRATCH

# Copy input files to $SCRATCH
echo "Copying input files to $SCRATCH"
cp /global/homes/j/jessd/amrex_LB/src/optimized_algorithms/inputs $SCRATCH

# Run the executable with different configurations
cd $SCRATCH

OUT_FOLDER=/global/homes/j/jessd/amrex_LB/output/$SLURM_JOB_NAME
SCRATCH_FOLDER=$SCRATCH/$SLURM_JOB_NAME
echo "Output folder: $SCRATCH_FOLDER"
# Create output folder if it doesn't exist
mkdir -p $SCRATCH_FOLDER

echo "Running the executable with different configurations"
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=1 domain="(256,256,256)" max_grid_size="(128,128,64)" >$SCRATCH_FOLDER/4_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=1 domain="(256,256,256)" max_grid_size="(128,64,64)" >$SCRATCH_FOLDER/4_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=1 domain="(256,256,256)" max_grid_size="(64,64,64)" >$SCRATCH_FOLDER/4_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=2 domain="(256,256,256)" max_grid_size="(128,64,64)" >$SCRATCH_FOLDER/8_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=2 domain="(256,256,256)" max_grid_size="(64,64,64)" >$SCRATCH_FOLDER/8_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=2 domain="(256,256,256)" max_grid_size="(64,64,32)" >$SCRATCH_FOLDER/8_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=4 domain="(256,256,256)" max_grid_size="(64,64,64)" >$SCRATCH_FOLDER/16_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=4 domain="(256,256,256)" max_grid_size="(64,64,32)" >$SCRATCH_FOLDER/16_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=4 domain="(256,256,256)" max_grid_size="(64,32,32)" >$SCRATCH_FOLDER/16_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=8 domain="(256,256,256)" max_grid_size="(64,64,32)" >$SCRATCH_FOLDER/32_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=8 domain="(256,256,256)" max_grid_size="(64,32,32)" >$SCRATCH_FOLDER/32_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=8 domain="(256,256,256)" max_grid_size="(32,32,32)" >$SCRATCH_FOLDER/32_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=16 domain="(256,256,256)" max_grid_size="(64,32,32)" >$SCRATCH_FOLDER/64_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=16 domain="(256,256,256)" max_grid_size="(32,32,32)" >$SCRATCH_FOLDER/64_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=16 domain="(256,256,256)" max_grid_size="(32,32,16)" >$SCRATCH_FOLDER/64_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=32 domain="(256,256,256)" max_grid_size="(32,32,32)" >$SCRATCH_FOLDER/128_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=32 domain="(256,256,256)" max_grid_size="(32,32,16)" >$SCRATCH_FOLDER/128_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=32 domain="(256,256,256)" max_grid_size="(32,16,16)" >$SCRATCH_FOLDER/128_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=64 domain="(256,256,256)" max_grid_size="(32,32,16)" >$SCRATCH_FOLDER/256_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=64 domain="(256,256,256)" max_grid_size="(32,16,16)" >$SCRATCH_FOLDER/256_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=64 domain="(256,256,256)" max_grid_size="(16,16,16)" >$SCRATCH_FOLDER/256_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=128 domain="(256,256,256)" max_grid_size="(32,16,16)" >$SCRATCH_FOLDER/512_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=128 domain="(256,256,256)" max_grid_size="(16,16,16)" >$SCRATCH_FOLDER/512_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=128 domain="(256,256,256)" max_grid_size="(16,16,8)" >$SCRATCH_FOLDER/512_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=256 domain="(256,256,256)" max_grid_size="(16,16,16)" >$SCRATCH_FOLDER/1024_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=256 domain="(256,256,256)" max_grid_size="(16,16,8)" >$SCRATCH_FOLDER/1024_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=256 domain="(256,256,256)" max_grid_size="(16,8,8)" >$SCRATCH_FOLDER/1024_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=512 domain="(256,256,256)" max_grid_size="(16,16,8)" >$SCRATCH_FOLDER/2048_4_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=512 domain="(256,256,256)" max_grid_size="(16,8,8)" >$SCRATCH_FOLDER/2048_8_output.txt
# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=512 domain="(256,256,256)" max_grid_size="(8,8,8)" >$SCRATCH_FOLDER/2048_16_output.txt

# ./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=1024 domain="(256,256,256)" max_grid_size="(16,8,8)" >$SCRATCH_FOLDER/4096_4_output.txt
./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=1024 domain="(256,256,256)" max_grid_size="(8,8,8)" >$SCRATCH_FOLDER/4096_8_output.txt
./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=1024 domain="(256,256,256)" max_grid_size="(8,8,4)" >$SCRATCH_FOLDER/4096_16_output.txt

./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=2048 domain="(256,256,256)" max_grid_size="(8,8,8)" >$SCRATCH_FOLDER/8192_4_output.txt
./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=2048 domain="(256,256,256)" max_grid_size="(8,8,4)" >$SCRATCH_FOLDER/8192_8_output.txt
./main3d.gnu.x86-milan.TPROF.OMP.ex inputs nnodes=2048 domain="(256,256,256)" max_grid_size="(8,4,4)" >$SCRATCH_FOLDER/8192_16_output.txt

# Copy output files back to the output folder
echo "Copying output files back to $OUT_FOLDER"
mkdir -p $OUT_FOLDER
cp $SCRATCH_FOLDER/*_output.txt $OUT_FOLDER

echo "Done!"
