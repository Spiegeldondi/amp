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

#define n_threads 4

void reset_arr(atomic_int* array, atomic_int value, atomic_int size)
{
    for (atomic_int i = 0; i < size; i++)
        array[i] = value;
}

void peterson_lock(atomic_int* flag, atomic_int* victim, int threadId) {
    atomic_int i = threadId;
    atomic_int j = 1 - i;

    flag[i] = 1;
    *victim = i;

    while (flag[j] && *victim == i) { /* a printf() in here or a pragma omp critical reduces the spread between shared counter and counts */ }
}

void peterson_unlock(atomic_int* flag, int threadId) {
    atomic_int i = threadId;
    flag[i] = 0;
}

void tree_lock(atomic_int* flag, atomic_int* victim, int threadId) {

    atomic_int levels = log2(n_threads);

    for (atomic_int l = 0; l < levels; l++) {
        atomic_int i = floor(threadId / pow(2, l));

        //atomic_int flag_offset = 2 * floor(threadId / pow(2, l + 1)) + (pow(2, l) - 1) / pow(2, l) * 2 * n_threads;
        atomic_int victim_offset = floor(i / 2) + (pow(2, l) - 1) / pow(2, l) * n_threads;
        atomic_int flag_offset = 2 * victim_offset;

        /* 2-thread Peterson lock */
        peterson_lock(&flag[flag_offset], &victim[victim_offset], i%2);
    }
}

void tree_unlock(atomic_int* flag, int threadId) {
    
    atomic_int levels = log2(n_threads);

    for (atomic_int l = 0; l < levels; l++) {
        atomic_int i = floor(threadId / pow(2, l));

        atomic_int flag_offset = 2 * floor(threadId / pow(2, l + 1)) + (pow(2, l) - 1) / pow(2, l) * 2 * n_threads;

        /* 2-thread Peterson unlock */
        peterson_unlock(&flag[flag_offset], i%2);
    }
}

int main(int argc, char **argv)
{
    int threadId;

    /* Peterson lock */
    atomic_int victim = n_threads + 1;

    atomic_int* flag;
    flag = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(flag, 0, n_threads);

    /* Tree lock */
    int n_locks = n_threads - 1;

    atomic_int* victim_tree;
    victim_tree = (atomic_int*) malloc(n_locks * sizeof(atomic_int));
    reset_arr(victim_tree, -1, n_locks);

    atomic_int* flag_tree;
    flag_tree = (atomic_int*) malloc(2 * n_locks * sizeof(atomic_int));
    reset_arr(flag_tree, 0 , 2 * n_locks);

    /* OpenMP reference */
    omp_lock_t baseline;
    omp_init_lock(&baseline);

    /* Correctness check */
    int shared_counter = 0;
    int local_counter = 0;
    int reference_counter = 0;

    /* Experiment setup */
    int shots = 100000;

    #pragma omp parallel private(threadId, local_counter) shared(flag, victim, flag_tree, victim_tree, shared_counter, reference_counter)
    {
        threadId = omp_get_thread_num();

        #pragma omp single
        {
            printf("\nnumber of threads: %d\n", omp_get_num_threads());
        }

        for (int s = 0; s < shots; s++) {
            
            tree_lock(flag_tree, victim_tree, threadId);
            //peterson_lock(flag, &victim, threadId);
            //omp_set_lock(&baseline);

            shared_counter += 1;
            local_counter += 1;

            tree_unlock(flag_tree, threadId);
            //peterson_unlock(flag, threadId);
            //omp_unset_lock(&baseline);

        }

        #pragma omp barrier
        
        #pragma omp critical
        reference_counter += local_counter;

    }

    omp_destroy_lock(&baseline);

    printf("\nshared counter: %d vs. expected value: %d and sum of local counters: %d\n", shared_counter, n_threads*shots, reference_counter);

    return 0;
}