#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include <stdatomic.h>

#define n_threads 8
#define flag_size 64

typedef atomic_int int_type_peterson; 
typedef atomic_int int_type_bw; 
typedef atomic_int int_type; // careful, Max value is 232!

void filter_lock(volatile int_type_peterson *level, volatile int_type_peterson *victim, int_type_peterson tid)
{
    // volatile int_type wait;
    for (int_type_peterson j = 1; j < n_threads; j++)
    {
        level[tid] = j;
        victim[j] = tid;
        for (int_type_peterson k = 0; k < n_threads; k++)
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

void filter_unlock(volatile int_type_peterson *level, int_type_peterson tid)
{
    level[tid] = 0;
}

int_type_bw sum_val_bw(volatile int_type_bw *values, int_type size)
{
    int_type sum = 0;
    for (int_type i = 0; i < size; i++)
    {
        sum += values[i];
    }
    return sum;
}

int_type sum_val(volatile int_type *values, int_type size)
{
    int_type sum = 0;
    for (int_type i = 0; i < size; i++)
    {
        sum += values[i];
    }
    return sum;
}

void block_woo_lock(volatile int_type_bw *competing, volatile int_type_bw *victim, int_type tid)
{
    int_type j = 0;
    competing[tid] = 1;
    do
    {
        j++;
        victim[j] = tid; // -->> victim is of type uint8_t, but tid is of type 
        while (!(victim[j] != tid || j >= sum_val_bw(competing, n_threads)))
        {
        };
        // printf("tid: %d victim[%d]: %d sum_val: %d\n", tid, j, victim[j], sum_val(competing, n_threads));
    } while (victim[j] != tid);
    // printf("tid: %d entereing CS at victim[%d]: %d \n", tid, j, victim[j]);
}

void block_woo_unlock(volatile int_type_bw *competing, int_type tid)
{
    competing[tid] = 0;
}

int_type alag_lock(volatile int_type *level, volatile int_type *competing, volatile int_type *victim, int_type tid)
{
    int_type j = 0;
    competing[tid] = 1;
    do
    {
        j++;
        level[tid] = j;
        victim[j] = tid;
        for (int_type k = 0; k < n_threads; k++)
        {
            if (k == tid)
                continue;
            while (!((victim[j] != tid) || ((level[k] < j) && j >= sum_val(competing, n_threads))))
            {
            };
        }
        // printf("tid: %d victim[%d]: %d sum_val: %d\n", tid, j, victim[j], sum_val(competing, n_threads));
    } while (victim[j] != tid);
    // printf("tid: %d entereing CS at victim[%d]: %d \n", tid, j, victim[j]);
    return j;
}

void alag_unlock(volatile int_type *competing, volatile int_type *level, volatile int_type *victim, int_type tid, int_type j)
{
    for (int_type k = 1; k < j; k++)
    {
        victim[j] = tid;
    }
    for (int_type k = 0; k < n_threads; k++)
    {
        if (k == tid)
            continue;
        while (!(level[k] == 0 || victim[level[k]] == k))
        {
        };
    }
    level[tid] = 0;
    competing[tid] = 0;
}

void peterson_lock(volatile int_type_peterson *flag, volatile int_type_peterson *victim, int_type_peterson tid, int_type_peterson level)
{
    int_type_peterson i = floor(tid / pow(2, level));    // tid has to be computed level wise
    int_type_peterson j = i + (i + 1) % 2 - i % 2;       // competitor
    int_type_peterson i_flag = i + (pow(2, level) - 1) / pow(2, level) * 2*n_threads;
    int_type_peterson j_flag = i_flag + (i + 1) % 2 - i % 2;
    int_type_peterson ij_victim =  floor(i/2)+(pow(2, level) - 1) / pow(2, level) *  n_threads; // works

    //printf("tid: %d level: %d\n", tid, level);
    //printf("i: %d j: %d \n", i, j);uint8_t
    //printf("i_flag: %d j_flag: %d ij_victim: %d \n", i_flag, j_flag, ij_victim);

    flag[i_flag] = 1;
    victim[ij_victim] = i;
    while (flag[j_flag] && victim[ij_victim] == i) {};
}

void peterson_unlock(volatile int_type_peterson *flag, int_type_peterson tid, int_type_peterson level)
{
    int_type i = floor(tid / pow(2, level));    // tid has to be computed level wise
    int_type i_flag = i + (pow(2, level) - 1) / pow(2, level) * 2*n_threads;

    flag[i_flag] =0;
}

void peterson_binary(volatile int_type_peterson *flag, volatile int_type_peterson *victim, int_type_peterson tid)
{
    int_type_peterson levels = log2(n_threads);
    //printf("tid: %d in level: %d", tid, levels);
    for (int_type_peterson level=0; level<levels; level++)
    {
        //printf("tid: %d in level: %d", tid, level);
        peterson_lock(flag, victim, tid, level);
    }
}

void peterson_release(volatile int_type_peterson *flag, int_type_peterson tid)
{
    int_type_peterson levels = log2(n_threads);
    for (int_type_peterson level=0; level<levels; level++)
    {
        peterson_unlock(flag, tid, level);
    }
}

double avg_val(double *values, int_type size)
{
    double sum = 0;
    for (int_type i = 0; i < size; i++)
    {
        sum += values[i];
    }
    return sum / size;
}

double std_dev(double *values, int_type size)
{
    double avg = avg_val(values, size);
    double sum_sq_diff = 0;
    for (int_type i = 0; i < size; i++)
    {
        double diff = values[i] - avg;
        sum_sq_diff += diff * diff;
    }
    return sqrt(sum_sq_diff / size);
}

void reset_arr(volatile int_type* array, int_type value, int size)
{
    for (int i=0; i < size; i++) array[i] = value;
}

void reset_arr_peterson(volatile int_type_peterson* array, int_type_peterson value, int size)
{
    for (int i=0; i < size; i++) array[i] = value;
}

void reset_arr_bw(volatile int_type_bw* array, int_type_bw value, int size)
{
    for (int i=0; i < size; i++) array[i] = value;
}


int main(int argc, char **argv)
{
    assert(232 > n_threads && "unint8_t OVERFLOW");

    volatile int_type level[n_threads];
    volatile int_type competing[n_threads] = {0};
    volatile int_type_peterson victim_peterson[flag_size], flag_peterson[flag_size], level_filter[n_threads], victim_filter[n_threads];

    volatile int_type_bw victim_woo[n_threads];
    volatile int_type_bw competing_bw[n_threads] = {0}; // need to set array to zero for sum_val to work
    

    int_type lock_log[n_threads] = {0};                 // stores order of access into CSQ[i]  by thread_ID
    int_type_peterson tid, index;
    index = 0;

    int shots = 1;
    double timings[shots];
    double timing_tid[n_threads]; 
    double timing_avg = 0;

    omp_lock_t baseline;
    omp_init_lock(&baseline);

    #pragma omp parallel private(tid) shared(level, level_filter, victim_filter, victim_woo, flag_peterson, victim_peterson, competing, lock_log, index, timing_tid, timings, shots)
    {
        #pragma omp single
        printf("Number of threads: %d, brought to you by %d\n", omp_get_num_threads(), omp_get_thread_num());

        tid = omp_get_thread_num();
        #pragma omp barrier
        #pragma omp barrier

        for (int_type s = 0; s < shots; s++)
        {     
            #pragma omp single 
            {
                memset(lock_log, 0, sizeof(lock_log));
                index = 0;

                reset_arr(level,0,n_threads);
                reset_arr(competing,0,n_threads);

                reset_arr_bw(victim_woo,231,n_threads);
                reset_arr_bw(competing_bw,0,n_threads);

                reset_arr_peterson(flag_peterson,(int_type_peterson)0,flag_size);
                reset_arr_peterson(victim_peterson,(int_type_peterson)231,flag_size);
                reset_arr_peterson(victim_filter,(int_type_peterson)231,n_threads);
                reset_arr_peterson(level_filter,(int_type_peterson)0,n_threads);
            }
            #pragma omp barrier
            #pragma omp barrier

            double start, stop;
            #pragma omp barrier
            #pragma omp barrier
            start = omp_get_wtime();

            //omp_set_lock(&baseline);
            // filter_lock(level_filter, victim_filter, tid);
            block_woo_lock(competing_bw, victim_woo, tid);
            //int_type level_tid = alag_lock(level, competing, victim_woo, tid);
            //peterson_binary(flag_peterson,victim_peterson,tid);

            // Critical section //////////
        
            lock_log[index] = tid;
            index += 1;
            //////////////////////////////

            //omp_unset_lock(&baseline);
            //peterson_release(flag_peterson, tid);
            // filter_unlock(level_filter, tid);
            block_woo_unlock(competing_bw, tid);
            //alag_unlock(competing, level, victim_woo, tid, level_tid);

            stop = omp_get_wtime();
            timing_tid[tid]= stop-start;

            #pragma omp barrier
            #pragma omp barrier

            #pragma omp single
            {
                assert(index == n_threads && "RACE CONDITION was witnessed");
                double max_runtime=0;
                for (int c = 0; c < n_threads; c++)
                    if (timing_tid[c] > max_runtime) {max_runtime = timing_tid[c];}
                timings[s] = max_runtime;
            }
        }
    }

    omp_destroy_lock(&baseline);

    // check for correctness
    // in case of race condition, index will likely be < n_threads
    assert(index == n_threads && "RACE CONDITION was witnessed");
    printf("Index: %d vs n_threads: %d\n", index, n_threads);

    // analyze order of access into cs
    printf("access order:\n");
    for (int_type n = 0; n < n_threads; n++)
    {
        printf("%d ", lock_log[n]);
    }
    printf("\n");

    printf("time: \n%.2f +/- %.2f\n", 10e6 * avg_val(timings, shots), 10e6 * std_dev(timings, shots));

    return 0;
}