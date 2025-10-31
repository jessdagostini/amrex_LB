#!/bin/bash

module load python

for d in `ls -d */`; do
    cd $d
    python3  $HOME/amrex_LB/Tools/lb_extractor/parse-time.py output.txt --out $HOME/amrex_LB/Tools/lb_extractor/simulation/${d//\//}_tmp_time.csv
    python3 $HOME/amrex_LB/Tools/lb_extractor/parse_lb_data.py diags/reducedfiles/LBC.txt 5200 $HOME/amrex_LB/Tools/lb_extractor/simulation/${d//\//}_tmp_eff.csv
    cd ..
done