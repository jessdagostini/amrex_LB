import subprocess
import os
import json

# Find all input files in the lb_inputs directory
input_dir = os.path.join(os.environ['HOME'], "lb_extractor", "lb_inputs", "imp_50")
result_dir = os.path.join(os.environ['HOME'], "lb_extractor", "lb_results")
json_files = [f for f in os.listdir(input_dir) if f.endswith('.json')]

# For each input file
for json_file in json_files:
    # Read data from the json file
    with open(os.path.join(input_dir, json_file), 'r') as f:
        data = json.load(f)

    # Generate the other input characteristics to run with amrex_LB
    amrex_input = f"""name = {data['input_name']}
domain = ({data['n_cells'][0]}, {data['n_cells'][1]})
max_grid_size = ({data['max_grid_size'][0]}, {data['max_grid_size'][1]})
nghost = (1, 0, 0) 
periodicity = (0, 0, 0)
nnodes = {data['nodes']}
ranks_per_node = 4
steps = {data['max_steps']}
intervals = {data['load_balance_intervals']}
tiny_profiler.print_threshold = 0
debug=True
amrex.v = 1
#DistributionMapping.verbose_mapper=1
file={data['file_path']}
mean = 100000
stdev = 250
ratio = {data['ratio']}"""

    input_file = os.path.join(result_dir, os.path.basename(json_file).replace('.json', '.inputs'))
    print(f"Creating input file: {input_file}")
    with open(input_file, "w") as fh:
        fh.writelines(amrex_input)

    # Run the amrex_LB script with the generated input file
    output_file = os.path.join(result_dir, os.path.basename(json_file).replace('.json', '.out'))
    metrics_file = os.path.join(result_dir, os.path.basename(json_file).replace('.json', '.csv'))
    binary_path = os.path.join(os.environ['HOME'], 'amrex_LB', 'src', 'optimized_algorithms', 'main3d.gnu.x86-milan.TPROF.OMP.ex')
    print(f"Running amrex_LB with input file: {input_file}")
    
    with open(output_file, "w") as out_fh, open(metrics_file, "w") as metrics_fh:
        process = subprocess.Popen([binary_path, input_file], stdout=out_fh, stderr=metrics_fh)

        process.wait()

        if process.returncode != 0:
            print(f"Error running amrex_LB for {json_file}. Check output in {output_file} and metrics in {metrics_file}.")