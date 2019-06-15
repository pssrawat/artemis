DESCRIPTION
This is the ARTEMIS code generator, that generates highly-optimized and tunable
CUDA code from an input DSL specification.

DEPENDENCIES
We tested the framework on ubuntu 16.04 and Red Hat Enterprise Linux Server release 6.7 using a 
Kepler K40c card, with GCC 5.3.0, and NVCC 8.0. The following are hardware requirements
for the framework:
1. flex >= 2.6.0 (2.6.0 tested)
2. bison >= 3.0.4 (3.0.4 tested)
3. cmake >= 3.8 (3.8 tested)
4. GCC version 4 (4.9.2 tested) or 5 (5.3.0 tested)
5. NVCC 8.0


STEPS TO INSTALL

1. Simply run 'make all' in the main directory. The makefile will create a 'stencilgen' executable.


COPYRIGHT

All files in this archive which do not include a prior copyright are by default
included in this tool and copyrighted 2019 Ohio State University.


MORE INFORMATION

For more information on how to add a new benchmark, see the docs/ folder or contact me at <rawat.15@osu.edu>
