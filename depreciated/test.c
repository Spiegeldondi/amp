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

#define n_threads 2

void reset_arr(atomic_int* array, atomic_int value, atomic_int size)
{
    for (atomic_int i = 0; i < size; i++) // 'i' and 'size' was not atomic
        array[i] = value;
}

void peterson_lock(atomic_int* flag, atomic_int* victim, int threadId) {
    int i = threadId;
    int j = 1 - i;

    flag[i] = 1;
    *victim = i;

    while (flag[j] && *victim == i) {}
}

void peterson_unlock(atomic_int* flag, int threadId) {
    int i = threadId;
    flag[i] = 0;
}

int main(int argc, char **argv)
{
    int threadId;

    atomic_int victim = n_threads + 1;

    atomic_int* flag;
    flag = (atomic_int*) malloc(n_threads * sizeof(atomic_int));
    reset_arr(flag, 0, n_threads);

    int shared_counter = 0;
    int local_counter = 0;

    int shots = 512;

    omp_lock_t baseline;
    omp_init_lock(&baseline);

    #pragma omp parallel private(threadId, local_counter) shared(victim, flag, shared_counter)
    {
        threadId = omp_get_thread_num();

        #pragma omp single
        {
            printf("\nnumber of threads: %d\n", omp_get_num_threads());
        }

        for (int s = 0; s < shots; s++) {
            
            peterson_lock(flag, &victim, threadId);
            //omp_set_lock(&baseline);

            shared_counter += 1;

            peterson_unlock(flag, threadId);
            //omp_unset_lock(&baseline);

        }
    }

    omp_destroy_lock(&baseline);

    printf("\nshare counter: %d vs. expected value: %d\n", shared_counter, n_threads*shots);

    return 0;
}