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

#define n_threads 8

void reset_arr(atomic_int* array, atomic_int value, atomic_int size)
{
    for (atomic_int i = 0; i < size; i++)
        array[i] = value;
}

atomic_int sum_arr(atomic_int *array, atomic_int size)
{
    atomic_int sum = 0;
    for (atomic_int i = 0; i < size; i++)
    {
        sum += array[i];
    }
    return sum;
}

atomic_int non_zero(atomic_int *array, atomic_int size)
{
    atomic_int count = 0;
    for (atomic_int i = 0; i < size; i++)
    {
        if(array[i] != 0) {
            count += 1;
        }
    }
    return count;
}

void alagarsamy_lock(atomic_int* TURN, atomic_int* Q, int threadId, int j) {
    int i = threadId;

    do {
        j += 1;
        Q[i] = j;
        TURN[j] = i;
        //printf("%d from thread: %d\n", j, threadId);
        for (atomic_int k = 0; k < n_threads; k++) {
            //printf("%d from thread: %d\n", j, threadId);
            if (k == i) continue;
            while ( !( (TURN[j]!=i) || ((Q[k]<j) && non_zero(Q, n_threads) <= j)) ) {}
            // es kommt zum deadlock sobal sich alle beteiligten threads in dem while loop befinden (für n_threads >= 3)
        }
    } while ( !(TURN[j] == i) );
}


void alagarsamy_unlock(atomic_int* TURN, atomic_int* Q, int threadId, int j) {
    int i = threadId;

    for (atomic_int k = 1; k <= j-1; k++) {
        TURN[k] = i; // typo? used k instead of j
    }
    for (atomic_int k = 0; k < n_threads; k++) {
        if (k == i) continue;
        while ( !( (Q[k]==0) || TURN[Q[k]]==k) ) {}
    }
    Q[i] = 0;
}


int main(int argc, char **argv)
{
    int threadId;

    /* Alagarsamy lock */
    int j = 0;

    atomic_int* Q;
    Q = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(Q, 0, n_threads);

    atomic_int* TURN;
    TURN = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(TURN, -1, n_threads);

    /* OpenMP reference */
    omp_lock_t baseline;
    omp_init_lock(&baseline);

    /* Correctness check */
    int shared_counter = 0;
    int local_counter = 0;

    /* Experiment setup */
    int shots = 100000;

    #pragma omp parallel private(threadId, local_counter, j) shared(TURN, Q, shared_counter)
    {
        threadId = omp_get_thread_num();

        #pragma omp single
        {
            printf("\nnumber of threads: %d\n", omp_get_num_threads());
        }

        for (int s = 0; s < shots; s++) {
            
            //omp_set_lock(&baseline);
            alagarsamy_lock(TURN, Q, threadId, j);

            //printf("\nthread %d entered CS\n", threadId);
            shared_counter += 1;

            //omp_unset_lock(&baseline);
            alagarsamy_unlock(TURN, Q, threadId, j);

        }

    }

    omp_destroy_lock(&baseline);

    printf("\nshared counter: %d vs. expected value: %d\n", shared_counter, n_threads*shots);

    return 0;
}