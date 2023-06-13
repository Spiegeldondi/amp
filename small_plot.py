import os
import re
import glob

import numpy as np
import matplotlib.pyplot as plt

##############################
# FUNCTIONS
##############################

def compute_distances_single_run(run, tid, n_threads):
    """
    np.array run:   1D array of integers resembling thread ID's in order of lock acquisitions
    int tid:        a given thread ID as integer for which to compute the gaps between lock acquisitions
    int n_threads:  the number of threads used for this run
    """
    indices = np.where(run == tid)[0] # np.where() returns a tuple
    if len(indices) > 0 and indices[0] > n_threads: 
        np.insert(indices, 0, tid)
    differences = np.diff(indices)
    return differences.tolist()   

def count_frequencies_singe_run(run, tid):
    count = np.count_nonzero(run == tid)
    return count

def compute_gaps_and_freqs(data, n_threads, n_locks, total_runs):
    fair_share = n_locks/n_threads
    frequencies = []
    gaps = [] # stores all distances between occurences of the same thread ID for all threads and all runs
    for run in range(total_runs):
            for tid in range(n_threads):
                distances = compute_distances_single_run(data[run], tid, n_threads)
                frequencies.append(abs(fair_share - count_frequencies_singe_run(data[run], tid)))
                gaps += distances
    return frequencies, gaps

def load_fair_data(path,lock_dict):
     # Path to output files and list of output files
    data_src = glob.glob(path + "/out_*")
    file_names = [file_name.split('/')[-1] for file_name in data_src]

    freq_dict = {}
    gaps_dict = {}

    which_lock = -1

    for file in file_names:
        # Load one output file
        data = np.loadtxt(os.path.join(path, file), dtype=int)

        # Extract metadata from filename and data shape
        numbers = re.findall(r'\d+', file)
        n_threads = int(numbers[0])
        if n_threads > 1:
            which_lock = int(numbers[1])
            n_locks = n_threads * n_threads
            total_runs = data.shape[0]

            # Print some metadata
            print(n_threads, "threads using", lock_dict[which_lock], "lock")
            print(total_runs, "runs with", data.shape[1], "lock acquisitions\n") 

            # Compute the fairness metrics
            frequencies, gaps = compute_gaps_and_freqs(data, n_threads, n_locks, total_runs)

            freq_dict[n_threads] = frequencies
            gaps_dict[n_threads] = gaps
        
    return freq_dict, gaps_dict, which_lock

def load_performance_data(path,TP_or_LAT,lock_dict):
    # Path to output files and list of output files
    data_src = glob.glob(path + "/"+TP_or_LAT+"_*")
    file_names = [file_name.split('/')[-1] for file_name in data_src]

    avg_dict = {}
    std_dict = {}

    which_lock = -1

    for file in file_names:
        # Load one output file
        data = np.loadtxt(os.path.join(path, file), dtype=float)

        # Extract metadata from filename and data shape
        numbers = re.findall(r'\d+', file)
        n_threads = int(numbers[0])
        which_lock = int(numbers[1])
        n_locks = n_threads * n_threads
        total_runs = data.shape[0]

        # Compute the fairness metrics
        avg, std = data.mean(), data.std()

        avg_dict[n_threads] = avg
        std_dict[n_threads] = std

    return avg_dict,std_dict

################################
# MAIN
################################

if __name__ == '__main__':

    lock_dict = {1: 'OMP', 2: 'Filter', 3: 'Block-Woo', 4: 'Tree', 5: 'Alagarsamy'}
    
    cwd = os.getcwd()
    paths = glob.glob(cwd+"/output/lock*")

    #paths= ["/output/output_nebula/amp23s20/lock3","/output/output_nebula/amp23s20/lock1"]
    
    for path in paths:
        # EXTRACT DATA
        freq_dict, gaps_dict, which_lock=load_fair_data(path, lock_dict)
        avg_dict_tp, std_dict_tp=load_performance_data(path,"TP",lock_dict)
        avg_dict_lat, std_dict_lat=load_performance_data(path,"LAT",lock_dict)

        # PLOT FAIR I
        gaps_dict = dict(sorted(gaps_dict.items()))
        fig, ax = plt.subplots(figsize=(7,3), dpi=180)
        ax.set_title("Fairness (Overtaking): " + lock_dict[which_lock] + " Lock")
        ax.boxplot(gaps_dict.values(), medianprops={'color': 'red', 'linewidth': 2})
        ax.set_yscale("log")
        ax.minorticks_on()
        ax.grid(which="both")
        ax.set_xticklabels(gaps_dict.keys())
        ax.set_xlabel("number of threads")
        ax.set_ylabel("waiting-for-lock period length")

        plt.tight_layout()
        plt.savefig("./plots/" + "F1_" + lock_dict[which_lock] + ".png", )

        # PLOT FAIR II
        freq_dict = dict(sorted(freq_dict.items()))

        fig, ax = plt.subplots(figsize=(7,3), dpi=180)
        ax.set_title("Fairness (CS Accesses): " + lock_dict[which_lock] + " Lock")
        ax.boxplot(freq_dict.values(), medianprops={'color': 'red', 'linewidth': 2})
        ax.minorticks_on()
        ax.grid(which="both")
        ax.set_xticklabels(freq_dict.keys())
        ax.set_xlabel("number of threads")
        ax.set_ylabel("deviation from fair lock distribution")

        plt.tight_layout()
        plt.savefig("./plots/" + "F2_" + lock_dict[which_lock] + ".png")

        # PLOT TP
        keys = list(avg_dict_tp.keys())
        values = list(avg_dict_tp.values())
        errors = list(std_dict_tp.values())
        plt.figure(figsize=(7, 3), dpi=180)
        # Plot the data with error bars
        plt.errorbar(keys, values, yerr=errors, fmt='o', capsize=4, color= 'red', ecolor='black')
        plt.title("Throughput: " + lock_dict[which_lock] + " Lock")
        # Set labels for the x-axis and y-axis
        plt.xlabel('number of threads')
        plt.ylabel('throughput (locks/s)')
        plt.minorticks_on()
        plt.grid(which="both", alpha=0.3)

        plt.tight_layout()
        plt.savefig("./plots/" + "TP" + "_" + lock_dict[which_lock] + ".png")

        # PLOT LAT
        keys = list(avg_dict_lat.keys())
        values = list(avg_dict_lat.values())
        errors = list(std_dict_lat.values())
        plt.figure(figsize=(7, 3), dpi=180)
        # Plot the data with error bars
        plt.errorbar(keys, values, yerr=errors, fmt='o', capsize=4, color= 'red', ecolor='black')
        plt.title("Latency: " + lock_dict[which_lock] + " Lock")
        # Set labels for the x-axis and y-axis
        plt.xlabel('number of threads')
        plt.yscale("log")
        plt.ylabel('latenvy (micro-s)')
        plt.minorticks_on()
        plt.grid(which="both", alpha=0.3)
        
        plt.tight_layout()
        plt.savefig("./plots/" + "LAT" + "_" + lock_dict[which_lock] + ".png")