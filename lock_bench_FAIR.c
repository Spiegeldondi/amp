#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include <stdatomic.h>
#include <unistd.h>

// set number of threads as 
#define n_threads 4

#define max_threads 64 
#define flag_size (max_threads-1)*2
#define inner_iterations n_threads*n_threads
#define outer_iterations 30

// typedef int atomic_int; // careful, Max value is 232!

/* Helper Functions */

atomic_int sum_val(atomic_int *values, atomic_int size)
{
    atomic_int sum = 0;
    for (atomic_int i = 0; i < size; i++)
    {
        sum += values[i];
    }
    return sum;
}

double avg_val(double *values, atomic_int size)
{
    double sum = 0;
    for (atomic_int i = 0; i < size; i++)
    {
        sum += values[i];
    }
    return sum / size;
}

double std_dev(double *values, atomic_int size)
{
    double avg = avg_val(values, size);
    double sum_sq_diff = 0;
    for (atomic_int i = 0; i < size; i++)
    {
        double diff = values[i] - avg;
        sum_sq_diff += diff * diff;
    }
    return sqrt(sum_sq_diff / size);
}

void reset_arr(atomic_int *array, atomic_int value, int size)
{
    for (int i = 0; i < size; i++)
        array[i] = value;
}

/* Filter Lock */

void filter_lock(atomic_int *level, atomic_int *victim, atomic_int tid)
{
    // atomic_int wait;
    for (atomic_int j = 1; j < n_threads; j++)
    {
        level[tid] = j;
        victim[j] = tid;
        for (atomic_int k = 0; k < n_threads; k++)
        {
            if (k == tid)
                continue;
            while (level[k] >= j && victim[j] == tid)
            {
            }; // works
        }
    }
    // return wait;
}

void filter_unlock(atomic_int *level, atomic_int tid)
{
    level[tid] = 0;
}

/* Bock-Woo Lock */

void block_woo_lock(atomic_int *competing, atomic_int *victim, atomic_int tid)
{
    atomic_int j = 0;
    competing[tid] = 1;
    do
    {
        j++;
        victim[j] = tid; // -->> victim is of type uint8_t, but tid is of type
        while (!(victim[j] != tid || j >= sum_val(competing, n_threads)))
        {
        };
        // printf("tid: %d victim[%d]: %d sum_val: %d\n", tid, j, victim[j], sum_val(competing, n_threads));
    } while (victim[j] != tid);
    // printf("tid: %d entereing CS at victim[%d]: %d \n", tid, j, victim[j]);
}

void block_woo_unlock(atomic_int *competing, atomic_int tid)
{
    competing[tid] = 0;
}

/* Peterson Tournament Binary Tree Lock */

void peterson_lock(atomic_int *flag, atomic_int *victim, atomic_int tid, atomic_int level)
{
    atomic_int i = floor(tid / pow(2, level)); // tid has to be computed level wise
    atomic_int j = i + (i + 1) % 2 - i % 2;    // competitor
    atomic_int i_flag = i + (pow(2, level) - 1) / pow(2, level) * 2 * n_threads;
    atomic_int j_flag = i_flag + (i + 1) % 2 - i % 2;
    atomic_int ij_victim = floor(i / 2) + (pow(2, level) - 1) / pow(2, level) * n_threads; // works

    // printf("tid: %d level: %d\n", tid, level);
    // printf("i: %d j: %d \n", i, j);uint8_t
    // printf("i_flag: %d j_flag: %d ij_victim: %d \n", i_flag, j_flag, ij_victim);

    flag[i_flag] = 1;
    victim[ij_victim] = i;
    while (flag[j_flag] && victim[ij_victim] == i)
    {
    };
}

void peterson_unlock(atomic_int *flag, atomic_int tid, atomic_int level)
{
    atomic_int i = floor(tid / pow(2, level)); // tid has to be computed level wise
    atomic_int i_flag = i + (pow(2, level) - 1) / pow(2, level) * 2 * n_threads;

    flag[i_flag] = 0;
}

void peterson_binary(atomic_int *flag, atomic_int *victim, atomic_int tid)
{
    assert(log2(n_threads) == round(log2(n_threads)) && "n_threads does not suffice requirement: n_threads == 2**n");
    atomic_int levels = log2(n_threads);
    // printf("tid: %d in level: %d", tid, levels);
    for (atomic_int level = 0; level < levels; level++)
    {
        // printf("tid: %d in level: %d", tid, level);
        peterson_lock(flag, victim, tid, level);
    }
}

void peterson_release(atomic_int *flag, atomic_int tid)
{
    atomic_int levels = log2(n_threads);
    for (atomic_int level = 0; level < levels; level++)
    {
        peterson_unlock(flag, tid, level);
    }
}



int main(int argc, char **argv)
{
    // Check if the correct number of arguments is provided
    if (argc != 3) {
        printf("Invalid number of arguments. Usage: %s <number threads> <lock to use>\n", argv[0]);
        return 1;
    }

    // receive number of threads as command line argument
    int size = atoi(argv[1]);
    int which_lock = atoi(argv[2]);

    // Check if the argument format is valid
    if (size <= 0) {
        printf("Invalid argument. The number of threads must be a positive integer.\n");
        return 1;
    }

    // Check if the argument format is valid
    if ((which_lock <= 0) || (which_lock > 4)) {
        printf("Invalid argument. Second argument must be either 1, 2, 3 or 4.\n");
        return 1;
    }

    // Filter lock registers
    atomic_int* level;
    level = (atomic_int*) malloc(size * sizeof(atomic_int));
    memset(level, 0, size * sizeof(atomic_int));

    atomic_int* victim;
    victim = (atomic_int*) malloc(size * sizeof(atomic_int));
    memset(victim, 231, size * sizeof(atomic_int));

    // Peterson tournament binary tree lock registers
    int locks_in_tree = (size-1);

    atomic_int* victim_peterson;
    victim_peterson = (atomic_int*) malloc(locks_in_tree * sizeof(atomic_int));
    memset(victim_peterson, 231, locks_in_tree * sizeof(atomic_int));

    atomic_int* flag_peterson;
    flag_peterson = (atomic_int*) malloc(2*locks_in_tree * sizeof(atomic_int));
    memset(flag_peterson, 0, 2*locks_in_tree * sizeof(atomic_int));

    // Block-Woo lock registers
    atomic_int* competing;
    competing = (atomic_int*) malloc(size * sizeof(atomic_int));
    memset(competing, 0, size * sizeof(atomic_int));



    atomic_int* local_counter_arr;
    local_counter_arr = (atomic_int*) malloc(size * sizeof(atomic_int));
    memset(local_counter_arr, 0, size * sizeof(atomic_int));

    assert(232 > n_threads && "unint8_t OVERFLOW");

    // printf("%d %d %d\n", n_threads, outer_iterations,inner_iterations); 
    // printf("iteration runtime: %f\n", iteration_runtime);

    for (int j = 0; j < outer_iterations; j++)
    {
        // Filter lock reset registers 
        memset(level, 0, size * sizeof(atomic_int));
        memset(victim, 231, size * sizeof(atomic_int));

        // Peterson tournament binary tree reset registers
        memset(victim_peterson, 231, locks_in_tree * sizeof(atomic_int));
        memset(flag_peterson, 0, 2*locks_in_tree * sizeof(atomic_int));

        // Block-Woo reset registers
        memset(competing, 0, size * sizeof(atomic_int));        

        atomic_int lock_acquisition_log[inner_iterations] = {231}; // stores order of access into CSQ[i]  by thread_ID
        atomic_int tid;

        int shared_counter = 0;
        int local_counter = 0;
        memset(local_counter_arr, 0, size * sizeof(atomic_int));

        // OpenMP lock as baseline
        omp_lock_t baseline;
        omp_init_lock(&baseline);

        #pragma omp parallel private(tid, local_counter) shared(level, victim, victim_peterson, flag_peterson, competing, lock_acquisition_log, shared_counter, local_counter_arr)
        {
            assert(omp_get_num_threads() == n_threads && "set n_threads correctly");
            tid = omp_get_thread_num();

            #pragma omp single
            {
                // Filter lock reset registers 
                memset(level, 0, size * sizeof(atomic_int));
                memset(victim, 231, size * sizeof(atomic_int));

                // Peterson tournament binary tree reset registers
                memset(victim_peterson, 231, locks_in_tree * sizeof(atomic_int));
                memset(flag_peterson, 0, 2*locks_in_tree * sizeof(atomic_int));

                // Block-Woo reset registers
                memset(competing, 0, size * sizeof(atomic_int));  

                shared_counter = 0;
            }
            
            local_counter = 0;

            #pragma omp barrier
            #pragma omp barrier

            while (shared_counter < (inner_iterations - n_threads))
            {
                switch (which_lock)
                {
                case 1:
                    omp_set_lock(&baseline);
                    break;
                case 2:
                    filter_lock(level, victim, tid);
                    break;
                case 3:
                    block_woo_lock(competing, victim, tid);
                    break;
                case 4:
                    peterson_binary(flag_peterson, victim_peterson, tid);
                    break;
                }

                /* Begin of critical section */

                // record order of lock acquisitions for fairness benchmark
                lock_acquisition_log[shared_counter] = tid;

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
                    peterson_release(flag_peterson, tid);
                    break;
                }
            }

            #pragma omp barrier
            #pragma omp barrier
            
            local_counter_arr[tid] = local_counter;

            #pragma omp barrier
            #pragma omp barrier

            /* Correctness check */
            #pragma omp single
            {
                atomic_int sum_local_counters = sum_val(local_counter_arr, n_threads);
                //assert(shared_counter == sum_local_counters && "ERROR: Counter mismatch");
                //assert(shared_counter == inner_iterations   && "ERROR: Race condition");
                //printf("\nshared counter: %d vs. sum local counters: %d\n", shared_counter, sum_local_counters);
                //printf("\nshared counter: %d vs. prescribed iterations: %d\n", shared_counter, inner_iterations-1);

                /* Print result to stdout.
                One row per experiment repetition */
                for (int i = 0; i < inner_iterations; i++) {
                    printf("%d ", lock_acquisition_log[i]);
                }
                printf("\n");
            }
        }
        omp_destroy_lock(&baseline);
    }

    // free allocated memory

    //printf("\nSuccess!\n");
    return 0;
}

/*

Problems:
Assertion shared_counter == sum_local_counters fails for binary tree lock
Assertion shared_counter == inner_iterations fails for all locks
Block-Woo lock with 2 threads is very unfair
    -> huge gap in rare case when doing many locks
    -> one thread completely left out when doing 2 x 2 locks
    
*/