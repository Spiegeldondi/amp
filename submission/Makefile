.PHONY: create_venv build create-folder small-bench small-plot
SHELL:=/bin/bash
OUTPUT_PATH := $(PWD)/output/done

small-plot: small-bench
	@echo -e "\nWaiting for slurm job to complete...\n"
	@echo -e "...by looking for $(OUTPUT_PATH) to be generated...\n"
	@SECONDS=0; \
	while [ ! -f $(OUTPUT_PATH) ]; do \
        	echo -ne "Slurm job in progress... $$SECONDS seconds\r"; \
        	sleep 1; \
    	done
	@echo -e "\nSlurm job completed. Running small-plot python script...\n"
	python3 small_plot.py

small-bench: create-folder
	@echo -e "\nStart slurm job...\n"
	sbatch benchmark.job

create-folder: build
	@echo -e "\nCreate directories...\n"
	rm -rf plots
	rm -rf output
	mkdir -p plots
	mkdir -p output

build: create_venv
	@echo -e "\nCompile .c files...\n"
	gcc lock_bench_FAIR.c -o fair -lm -fopenmp
	gcc lock_bench_TP.c -o tp -lm -fopenmp
	gcc lock_bench_LAT.c -o lat -lm -fopenmp

create_venv:
	@echo -e "\nCreate python virtual environment...\n"
	@python3 -m venv myenv
	
	@echo -e "Activate python virtual environment...\n"
	@source myenv/bin/activate
	
	@echo -e "Upgrade pip and install numpy and matplotlib...\n"
	
	@pip install --upgrade pip
	@pip install numpy matplotlib

