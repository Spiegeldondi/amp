.PHONY: small-bench small-plot #small-bench and small-plot are treated as commands
SHELL:= /bin/bash

small-bench: build compile create-folder
        sbatch benchmark.job

small-plot: small-bench venv
        python3 small_plot.py

create-folder:
        rm -rf plots
        rm -rf output
        mkdir -p plots
        mkdir -p output

build:
        gcc lock_bench_FAIR.c -o fair -lm -fopenmp
        gcc lock_bench_TP.c -o tp -lm -fopenmp
        gcc lock_bench_LAT.c -o lat -lm -fopenmp

compile:
        source /opt/spack/spack_git/share/spack/setup-env.sh
        module load gcc-12.1.0-gcc-8.3.0-ixxzpor

venv:
        python -m venv venv
        source /venv/bin/activate
        python3 -m pip install --upgrade pip
        pip install matplotlib