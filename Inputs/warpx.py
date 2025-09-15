import reframe as rfm
import reframe.utility.sanity as sn

@rfm.simple_test
class warpx(rfm.RegressionTest):
    size = parameter(['2node','32node','960node'])
    def __init__(self):
        super().__init__()

        self.descr = "run warpx test"

        self.valid_prog_environs = ["default"]
        self.maintainers = ["kngott@lbl.gov"]

        self.modules = ["cmake"]
        self.env_vars = {"AMREX_CUDA_ARCH": "8.0", 'SLURM_CPU_BIND': 'cores'}
        self.time_limit = "10m"

        self.prebuild_cmds = ['git clone -b 25.03 https://github.com/ECP-WarpX/WarpX.git','cd WarpX']
        self.build_system = 'CMake'
        self.build_system.max_concurrency = 16
        self.build_system.builddir = 'build'
        self.build_system.config_opts = ['-DCMAKE_CUDA_HOST_COMPILER=CC -DWarpX_COMPUTE=CUDA -DWarpX_DIMS=3 -DWarpX_QED=OFF -DWarpX_EB=OFF']
        if self.size == "2node":
            self.num_tasks = 8
            self.executable_opts = ["inputs_small"]
            self.valid_systems = ["+gpu"]
            self.tags = {"n9nesap", "daily", "warpx","userstress"}
        if self.size == "32node":
            self.valid_systems = ["perlmutter:gpu"]
            self.num_tasks = 128
            self.executable_opts = ["inputs_large"]
            self.tags = {"n9nesap", "daily", "warpx","userstress"}
        if self.size == "240node":
            self.valid_systems = ["perlmutter:gpu"]
            self.num_tasks = 960
            self.executable_opts = ["inputs_240"]
            self.tags = {"userstress", "warpx", "weekly"}
        if self.size == "960node":
            self.valid_systems = ["perlmutter:gpu"]
            self.num_tasks = 3840
            self.executable_opts = ["inputs_960"]
            self.tags = {"userstress", "warpx", "monthly"}
        self.num_tasks_per_node = 4
        self.num_gpus_per_node = 4
        self.num_cpus_per_task = 32
        self.extra_resources = {'gpu_bind_type': {'gpu_bind_word': 'none'}}
        self.executable = "WarpX/build/bin/warpx.3d"

        self.sanity_patterns = sn.assert_found("Total Time", self.stdout)

        # Extract the performance figure of merit from stdout.
        self.perf_patterns = {
            "total_time": sn.extractsingle(r"Total Time\s+:\s+(\d+.\d+)", self.stdout, 1, float)
        }

        if self.size == "960node":
            self.reference = {
                "perlmutter:gpu" : {"total_time": (80, -1, 1e8, "s")},
                "muller:gpu" : {"total_time": (75, -1, 1e8, "s")}
            }
        if self.size == "240node":
            self.reference = {
                "perlmutter:gpu" : {"total_time": (80, -1, 1e8, "s")},
                "muller:gpu" : {"total_time": (75, -1, 1e8, "s")}
            }
        if self.size == "32node":
            self.reference = {
                "perlmutter:gpu" : {"total_time": (80, -1, 0.1, "s")},
                "muller:gpu" : {"total_time": (80, -1, 0.1, "s")}
            }
        if self.size == "2node":
            self.reference = {
                "perlmutter:gpu" : {"total_time": (75, -1, 0.1, "s")},
                "muller:gpu" : {"total_time": (75, -1, 0.1, "s")}
            }


