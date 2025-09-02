# AMReX Load Balancing Development, Testing & Exploration Repo

## **Introduction**
This repo is used to develop, test and evaluate new load balancing algorithms and strategies,
specifically for the framework, AMReX.

It includes varieties and variations of load balancing algorithms, codes to test and compare
algorithms on different weights and weight distributions, Jupyter notebooks and other tools 
to evaluate and explore the load balancing state of AMReX codes and code to collect and print
load balancing metrics and weights from AMReX applications.


## **Folder Structure**

```
amrex_LB
│
└─── notebooks                               <- jupyter-notebooks to plot all the obtained outputs         
│
└───output                                   <- this directory stores all the outputs (run_100 and run_250 
│                                               are no of times we ran the optimized algorithms.)                            
│   
└───result                                   <- this directory stores all the analysis plots  
│
└───src
│    │ 
│    └───bruteForce                          <-  bruteforce experimentation     
│    │ 
│    │  
│    └───optimized_algorithm                  <- existing and developed dynamic load balancing algorithms
|
└───LICENSE
└───README.md
    
```

## **Prerequisites**

### **System Requirements**

- **Operating System:** macOS, Linux, or similar Unix-based systems
- **Compiler:** GCC (version 9.0 or later) or Clang supporting C++17 or later.
- **Build Tools:** make utility for building the code.
- **Shell:** Bash for running shell scripts.

### **Dependencies**

- **AMReX :** A software framework for building parallel AMR applications.
- **CMake:** Version 3.15 or later for configuring the build system.
- **MPI:** OpenMPI for parallel execution (if required). 


## **Getting Started**

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### **Installation**

1. First clone the repository to your local machine:
   ```
   git clone https://github.com/kngott/amrex_LB.git

   cd ACM_PEARC_2025_Paper_Artifact

   ```
2. Install and Build AMReX from the step here:

   https://amrex-codes.github.io/amrex/docs_html/BuildingAMReX_Chapter.html

   Clone this repository to the working directory

   ```
   git clone https://github.com/AMReX-Codes/amrex.git

   ```
   This repository includes all ```amrex``` functionalities required to run our test-cases. So modify the ```GNUmakefile``` accordingly. Change the ```AMREX_HOME``` path to the correct path of cloned ```amrex``` repository. 

   ```
   AMREX_HOME ?= ../../../amrex

   ```

### **Build Instructions for bruteforce algorithm**

1. Navigate to the ```bruteForce``` directory

   ```
   CC bruteForce.cpp -o bf -fopenmp

   ```

### **Build Instructions for optimized algorithms**

1. Navigate to the ```optimized_algorithms``` directory

   ```
   cd /path/to/amrex_LB/src/optimized_algorithms/

   ```
2. Configure the Makefile:

   Ensure your ```Make.package``` file includes the correct paths to ```AMReX``` headers and libraries also all files headers and sources. 

3. Compile the Code:

   ```
   make -j 

   ```
   This will generate the executable ```main3d.gnu.x86-milan.TPROF.ex ```

### **Run Instructions**

1. Navigate to the Repository Root:

   Change the ```inputs``` as per your requirement. You can change the ```mean```, ```standard_deviation```, ```no_of_runs```, ```domain_size```, ```max_grid_size``` . 

2. Run the experiment file

   ```
   ./experiment.sh

   ```
3. Direct Execution

   ```
   ./main3d.gnu.x86-milan.TPROF.ex inputs nnodes=2 domain="(256,256,256)" max_grid_size= "(128,128,128)" > output/2_4_output_worst.txt

   ```

4. Running a bash file through jobs on perlmutter

   ```
   sbatch run.sh 

   ```

## **License**

This repo uses the same license as the AMReX repo.

## **Authors**
Kevin Gott \
Md Kamal Chowdhury\
Amitash Nanda \
Hannah Ross \
Sowmya Yellapragada

Plus, the BoxLib and AMReX authors of the original load balancing algorithms.

## **Acknowledgments**
1. **AMReX Team**
2. **Perlmutter Supercomputer**
3. **Lawrence Berkeley National Laboratory**
4. **National Energy Research Scientific Computing Center**
5. **University of California San Diego (Boolean Lab)**
6. **The University of Alabama**
7. **University of Utah**
