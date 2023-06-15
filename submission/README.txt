################
# Requirements #
################

*** important ***

Please make sure to use gcc version 12.1.0 installed on the cluster by either issuing:

>> spack load gcc@12.1.0

or:

>> source /opt/spack/spack_git/share/spack/setup-env.sh
>> module load gcc-12.1.0-gcc-8.3.0-ixxzpor

and verifying by:

>> gcc --version

expected output:

gcc (Spack GCC) 12.1.0
Copyright (C) 2022 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Otherwise lock 5 will sometimes crash and no outputs and plots can be produced!

###############################
# Contents & Folder Structure #
###############################

submission/
- lock_bench_FAIR.c
- lock_bench_LAT.c
- lock_bench_TP.c
- locks.h
- small_plot.py
- benchmark.job
- Makefile
- README.txt

*** the following folders are created uppon execution of the Makefile ***

- output/
	- lock1/
		...
	- lock2/
		...
	- lock3/
		...
	- lock4/
		...
	- lock5/
		...
- plots/
	...

###############
# Description #
###############

locks.h: This header file contains the source code for the locks used in the three lock_bench_*.c files

lock_bench_*.c: Three different files running different locks multiple times and log fairness (FAIR),
				latency (LAT) and throughput (TP) respectively and write the output into the respective
				lock*/ folder inside the output/ folder
				
benchmark.job: A slurm script that runs the compiled lock_bench_*.c files (compiled to "fair", "lat" and "tp")
			   iteratively for different numbers of threads and for each lock (1 to 5) separately.
			   
small-plot.py: This python script reads in the data from the output/ folder and its subfolders and creates plots
			   similar to the ones in the report. Note that for this script to work, the python libraries "numpy"
			   and "matplotlib" have to be installed. This is ensured in the Makefile by creating a virtual environment
			   and installing both dependencies. See next point.
			   
Makefile: First creates a python virtual environment and installs "numpy" and "matplotlib" into it.
		  These are requirements for the small-plot.py script to generate and save plots into the plots/ folder
		  using the data saved to the output/ folder by the compiled lock_bench_*.c files.
		  
##############
# How to run #
##############

*** important ***

From within the submission directory:

>> make

#############
# Debugging #
#############

The Makefile is constructed in such a way, that after all the setup (compiling, creating virtual environment) the 
benchmark.job script is started. The Makefile continuously checks for the creation of ./output/done, which is a small
textfile indicating that the slurm jobs have completed running and produced all the output. Only then the small-plot.py 
is executed. The small-plot.py script will only work IFF all data from the benchmark.job script has been produced, thus 
the waiting period. Multiple test runs have shown, that this executes in < 60 seconds.

