#!/bin/bash
set -e

echo "-------------------- NVCC ---------------------"

time=`grep -E 'float|double' nvcc-orig-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
awk -v otime=$time 'BEGIN {print "Original GFlops = " (300*300*300*358/10^6/otime)}'

timea=`grep -E 'float|double' nvcc-reorder-results-a | awk 'BEGIN {timea = 0.0} {timea += $2} END {print timea}'`
timeb=`grep -E 'float|double' nvcc-reorder-results-b | awk 'BEGIN {timeb = 0.0} {timeb += $2} END {print timeb}'`
awk -v atime=$timea -v btime=$timeb 'BEGIN {print "Reordered GFlops = " (300*300*300*358/10^6/(atime<btime?atime:btime))}'

echo "-------------------- LLVM ---------------------"

time=`grep -E 'float|double' llvm-orig-results | awk 'BEGIN {time = 0.0} {time += $2} END {print time}'`
awk -v otime=$time 'BEGIN {print "Original GFlops = " (300*300*300*358/10^6/otime)}'

timea=`grep -E 'float|double' llvm-reorder-results-a | awk 'BEGIN {timea = 0.0} {timea += $2} END {print timea}'`
timeb=`grep -E 'float|double' llvm-reorder-results-b | awk 'BEGIN {timeb = 0.0} {timeb += $2} END {print timeb}'`
awk -v atime=$timea -v btime=$timeb 'BEGIN {print "Reordered GFlops = " (300*300*300*358/10^6/(atime<btime?atime:btime))}'
