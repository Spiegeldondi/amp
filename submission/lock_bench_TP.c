#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>
#include "locks.h"

#define outer_iterations 10

int main(int argc, char **argv)
{
    // Check if the correct number of arguments is provided
    if (argc != 3) {
        printf("Invalid number of arguments. Usage: %s <number threads> <lock to use>\n", argv[0]);
        return 1;
    }

    // receive number of threads as command line argument
    int n_threads = atoi(argv[1]);
    int inner_iterations = n_threads*n_threads + n_threads; // + n_threads as safety buffer
    int which_lock = atoi(argv[2]);

    // Check if the argument format is valid
    if (n_threads <= 0) {
        printf("Invalid argument. The number of threads must be a positive integer.\n");
        return 1;
    }

    // Check if the argument format is valid
    if ((which_lock <= 0) || (which_lock > 5)) {
        printf("Invalid argument. Second argument must be either 1, 2, 3, 4 or 5.\n");
        return 1;
    }

    // Filter lock registers
    atomic_int* level;
    level = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(level, 0, n_threads);

    atomic_int* victim;
    victim = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(victim, 231, n_threads);

    // Peterson tournament binary tree lock registers
    int n_locks = pow(2,ceil(log2(n_threads)))- 1;

    atomic_int* victim_tree;
    victim_tree = (atomic_int*) malloc(n_locks * sizeof(atomic_int));
    reset_arr(victim_tree, -1, n_locks);

    atomic_int* flag_tree;
    flag_tree = (atomic_int*) malloc(2 * n_locks * sizeof(atomic_int));
    reset_arr(flag_tree, 0 , 2 * n_locks);

    // Block-Woo lock registers
    atomic_int* competing;
    competing = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(competing, 0, n_threads);

    // Alagarsamy lock
    int l = 0;

    atomic_int* Q;
    Q = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(Q, 0, n_threads);

    atomic_int* TURN;
    TURN = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(TURN, -1, n_threads);



    atomic_int* local_counter_arr;
    local_counter_arr = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(local_counter_arr, 0, n_threads);

    assert(232 > n_threads && "unint8_t OVERFLOW");

    for (int j = 0; j < outer_iterations; j++)
    {
        // Filter lock reset registers 
        reset_arr(level, 0, n_threads * sizeof(atomic_int));
        reset_arr(victim, 231, n_threads);

        // Peterson tournament binary tree reset registers
        reset_arr(victim_tree, 231, n_locks * sizeof(atomic_int));
        reset_arr(flag_tree, 0, 2*n_locks);

        // Block-Woo reset registers
        reset_arr(competing, 0, n_threads);  
        
        atomic_int tid;

        int shared_counter = 0;
        int local_counter = 0;
        reset_arr(local_counter_arr, 0, n_threads);

        // Timing
        double time_1, time_2;

        // OpenMP lock as baseline
        omp_lock_t baseline;
        omp_init_lock(&baseline);

        #pragma omp parallel private(tid, local_counter, l, time_1, time_2) shared(level, victim, victim_tree, flag_tree, competing, shared_counter, local_counter_arr)
        {
            assert(omp_get_num_threads() == n_threads && "set n_threads correctly");
            tid = omp_get_thread_num();

            #pragma omp single
            {
                // Filter lock reset registers 
                reset_arr(level, 0, n_threads);
                reset_arr(victim, 231, n_threads);

                // Peterson tournament binary tree reset registers
                reset_arr(victim_tree, 231, n_locks);
                reset_arr(flag_tree, 0, 2*n_locks);

                // Block-Woo reset registers
                reset_arr(competing, 0, n_threads);  

                shared_counter = 0;
            }
            
            local_counter = 0;

            #pragma omp barrier
            #pragma omp barrier

            if (tid == 0)
            {
                time_1 = omp_get_wtime();
            }

            #pragma omp barrier
            #pragma omp barrier

            while (shared_counter < n_threads*n_threads - 1)
            {
                switch (which_lock)
                {
                case 1:
                    omp_set_lock(&baseline);
                    break;
                case 2:
                    filter_lock(level, victim, tid, n_threads);
                    break;
                case 3:
                    block_woo_lock(competing, victim, tid, n_threads);
                    break;
                case 4:
                    tree_lock(flag_tree, victim_tree, tid, n_threads);
                    break;
                case 5:
                    alagarsamy_lock(TURN, Q, tid, l, n_threads);
                    break;
                }

                /* Begin of critical section */

                // increment counters for correctness check
                shared_counter += 1;
                local_counter  += 1;

                /* End of critical section */
                
                switch (which_lock)
                {
                case 1:
                    omp_unset_lock(&baseline);
                    break;
                case 2:
                    filter_unlock(level, tid);
                    break;
                case 3:
                    block_woo_unlock(competing, tid);
                    break;
                case 4:
                    tree_unlock(flag_tree, tid, n_threads);
                    break;
                case 5:
                    alagarsamy_unlock(TURN, Q, tid, l, n_threads);
                    break;
                }
            }

            #pragma omp barrier
            #pragma omp barrier

            if (tid == 0)
            {
                time_2 = omp_get_wtime();
                double tp = shared_counter / (time_2 - time_1);
                printf("%f\n", tp);
            }
            
            local_counter_arr[tid] = local_counter;

            #pragma omp barrier
            #pragma omp barrier

            /* Correctness check */
            #pragma omp single
            {
                atomic_int sum_local_counters = sum_val(local_counter_arr, n_threads);
                if (which_lock != 4) {
                    assert(shared_counter == sum_local_counters && "ERROR: Counter mismatch");
                }
            }
        }
        omp_destroy_lock(&baseline);
    }

    // free allocated memory

    return 0;
}
