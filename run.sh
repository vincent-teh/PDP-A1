#!/bin/bash -l
# You must specify a project
#SBATCH -A "uppmax2025/2-247"
#SBATCH -J serialtest
# Put all output in the file hello.out
#SBATCH -o result.out
# request 5 seconds of run time
#SBATCH -t 0:0:5
# request one core
#SBATCH -p node -n 8
start_time=$(date +%s%N)

mpirun -np 8 ./a.out 10000000

end_time=$(date +%s%N)
elapsed_time=$(awk "BEGIN {printf \"%.6f\", ($end_time - $start_time) / 1000000000}")
echo "Elapsed time: $elapsed_time seconds"
