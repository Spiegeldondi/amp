.PHONY: small-bench small-plot #small-bench and small-plot are treated as commands

small-bench: build create-folder
	sbatch benchmark.job

small-plot: small-bench
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


