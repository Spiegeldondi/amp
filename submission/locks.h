#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdatomic.h>
#include <unistd.h>

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

void reset_log(int* array, int value, int size)
{
    for (int i=0; i < size; i++) array[i] = value;
}

/* Filter Lock */

void filter_lock(atomic_int *level, atomic_int *victim, atomic_int tid, int n_threads)
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

void block_woo_lock(atomic_int *competing, atomic_int *victim, atomic_int tid, int n_threads)
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

void tree_lock(atomic_int* flag, atomic_int* victim, int threadId, int n_threads) {

    atomic_int levels = ceil(log2(n_threads));

    for (atomic_int l = 0; l < levels; l++) {
        atomic_int i = floor(threadId / pow(2, l));

        atomic_int flag_offset = 2 * floor(threadId / pow(2, l + 1)) + (pow(2, l) - 1) / pow(2, l) * 2 * pow(2, levels);//n_threads;
        atomic_int victim_offset = floor(i / 2) + (pow(2, l) - 1) / pow(2, l) * pow(2, levels); //n_threads;

        /* 2-thread Peterson lock */
        peterson_lock(&flag[flag_offset], &victim[victim_offset], i%2);
    }
}

void tree_unlock(atomic_int* flag, int threadId, int n_threads) {
    
    atomic_int levels = ceil(log2(n_threads));

    for (atomic_int l = 0; l < levels; l++) {
        atomic_int i = floor(threadId / pow(2, l));

        atomic_int flag_offset = 2 * floor(threadId / pow(2, l + 1)) + (pow(2, l) - 1) / pow(2, l) * 2 * pow(2, levels); //n_threads;

        /* 2-thread Peterson unlock */
        peterson_unlock(&flag[flag_offset], i%2);
    }
}

/* Alagarsamy lock */

void alagarsamy_lock(atomic_int* TURN, atomic_int* Q, int threadId, int j, int n_threads) {
    int i = threadId;

    do {
        j += 1;
        Q[i] = j;
        TURN[j] = i;
        //printf("%d from thread: %d\n", j, threadId);
        for (atomic_int k = 0; k < n_threads; k++) {
            //printf("%d from thread: %d\n", j, threadId);
            if (k == i) continue;
            while ( !( (TURN[j]!=i) || ((Q[k]<j) && (non_zero(Q, n_threads) <= j))) ) {}
            // es kommt zum deadlock sobal sich alle beteiligten threads in dem while loop befinden (für n_threads >= 3)
        }
    } while ( !(TURN[j] == i) );
}

void alagarsamy_unlock(atomic_int* TURN, atomic_int* Q, int threadId, int j, int n_threads) {
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
