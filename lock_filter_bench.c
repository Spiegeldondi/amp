#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include <stdatomic.h>
#include <unistd.h>

#define n_threads 8
#define flag_size 64
#define inner_iterations 100
#define outer_iterations 10

typedef atomic_int int_type; // careful, Max value is 232!
// typedef int int_type; // careful, Max value is 232!

/* Helper Functions */

int_type sum_val(int_type *values, int_type size)
{
    int_type sum = 0;
    for (int_type i = 0; i < size; i++)
    {
        sum += values[i];
    }
    return sum;
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

void reset_arr(int_type *array, int_type value, int size)
{
    for (int i = 0; i < size; i++)
        array[i] = value;
}

/* Filter Lock */

void filter_lock(int_type *level, int_type *victim, int_type tid)
{
    // int_type wait;
    for (int_type j = 1; j < n_threads; j++)
    {
        level[tid] = j;
        victim[j] = tid;
        for (int_type k = 0; k < n_threads; k++)
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

void filter_unlock(int_type *level, int_type tid)
{
    level[tid] = 0;
}

/* Bock-Woo Lock */

void block_woo_lock(int_type *competing, int_type *victim, int_type tid)
{
    int_type j = 0;
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

void block_woo_unlock(int_type *competing, int_type tid)
{
    competing[tid] = 0;
}

/* Peterson Tournament Binary Tree Lock */

void peterson_lock(int_type *flag, int_type *victim, int_type tid, int_type level)
{
    int_type i = floor(tid / pow(2, level)); // tid has to be computed level wise
    int_type j = i + (i + 1) % 2 - i % 2;    // competitor
    int_type i_flag = i + (pow(2, level) - 1) / pow(2, level) * 2 * n_threads;
    int_type j_flag = i_flag + (i + 1) % 2 - i % 2;
    int_type ij_victim = floor(i / 2) + (pow(2, level) - 1) / pow(2, level) * n_threads; // works

    // printf("tid: %d level: %d\n", tid, level);
    // printf("i: %d j: %d \n", i, j);uint8_t
    // printf("i_flag: %d j_flag: %d ij_victim: %d \n", i_flag, j_flag, ij_victim);

    flag[i_flag] = 1;
    victim[ij_victim] = i;
    while (flag[j_flag] && victim[ij_victim] == i)
    {
    };
}

void peterson_unlock(int_type *flag, int_type tid, int_type level)
{
    int_type i = floor(tid / pow(2, level)); // tid has to be computed level wise
    int_type i_flag = i + (pow(2, level) - 1) / pow(2, level) * 2 * n_threads;

    flag[i_flag] = 0;
}

void peterson_binary(int_type *flag, int_type *victim, int_type tid)
{
    assert(log2(n_threads) == round(log2(n_threads)) && "n_threads does not suffice requirement: n_threads == 2**n");
    int_type levels = log2(n_threads);
    // printf("tid: %d in level: %d", tid, levels);
    for (int_type level = 0; level < levels; level++)
    {
        // printf("tid: %d in level: %d", tid, level);
        peterson_lock(flag, victim, tid, level);
    }
}

void peterson_release(int_type *flag, int_type tid)
{
    int_type levels = log2(n_threads);
    for (int_type level = 0; level < levels; level++)
    {
        peterson_unlock(flag, tid, level);
    }
}

int main(int argc, char **argv)
{
    assert(232 > n_threads && "unint8_t OVERFLOW");
    printf("n_threads %d\n", n_threads);
    // printf("iteration runtime: %f\n", iteration_runtime);
    printf("iterations %d x %d\n", outer_iterations,inner_iterations);

    for (int j = 0; j < outer_iterations; j++)
    {
        int_type level[n_threads] = {0};
        int_type competing[n_threads] = {0};
        int_type victim[n_threads] = {231};

        int_type victim_peterson[flag_size] = {231}, flag_peterson[flag_size] = {0};

        int_type lock_log[inner_iterations] = {231}; // stores order of access into CSQ[i]  by thread_ID
        int_type tid;
        int index = 0;
        int th_index = 0; // WHY DOESNT THIS DO ANYTHING

        double iteration_start;
        double max_times[n_threads];
        double min_times[n_threads];
        double avg_times[n_threads];
        int_type th_indices[n_threads];

        omp_lock_t baseline;
        omp_init_lock(&baseline);

        #pragma omp parallel private(tid, th_index) shared(iteration_start, level, victim, competing, victim_peterson, flag_peterson, lock_log, index, max_times, min_times, avg_times, th_indices)
        
        {
            assert(omp_get_num_threads() == n_threads && "set n_threads correctly");
            tid = omp_get_thread_num();
            double max_time = 0.0;
            double avg_time = 0.0;
            double min_time = 10 * 1e6;

            #pragma omp single
            {
                reset_arr(level, 0, n_threads);
                reset_arr(competing, 0, n_threads);
                reset_arr(victim, 231, n_threads);

                reset_arr(flag_peterson, 0, flag_size);
                reset_arr(victim_peterson, 231, flag_size);

                iteration_start = omp_get_wtime();
            }

            th_index = 0; // does only take effect here, not outside the parallel region
            #pragma omp barrier
            #pragma omp barrier

            // while ((omp_get_wtime()-iteration_start) < iteration_runtime)
            // for (int_type s = 0; s < shots; s++)
            while (index < (inner_iterations - n_threads))
            {
                // #pragma omp barrier // NECESSARY??
                // #pragma omp barrier // NECESSARY??

                double time_1 = omp_get_wtime();

                // omp_set_lock(&baseline);
                filter_lock(level, victim, tid);
                // block_woo_lock(competing, victim, tid);
                // peterson_binary(flag_peterson, victim_peterson, tid);

                //////////////////////////////
                // Critical section //////////

                double time_2 = omp_get_wtime();
                double time_dif = (time_2 - time_1) * 1e6; // micro seconds
                avg_time += time_dif;

                if (time_dif > max_time)
                    max_time = time_dif;
                if (time_dif < min_time)
                    min_time = time_dif;

                lock_log[index] = tid;

                index += 1;
                th_index += 1;

                //////////////////////////////
                //////////////////////////////

                // omp_unset_lock(&baseline);
                filter_unlock(level, tid);
                // block_woo_unlock(competing, tid);
                // peterson_release(flag_peterson, tid);
            }

            // COLLECT DATA
            #pragma omp barrier
            #pragma omp barrier

            min_times[tid] = min_time;
            max_times[tid] = max_time;
            avg_times[tid] = avg_time / th_index;
            th_indices[tid] = th_index;

            /////////////////////////////////////
            // STATISTICS
            /////////////////////////////////////
            #pragma omp barrier
            #pragma omp barrier

            #pragma omp single
            {
                int_type real_index = sum_val(th_indices, n_threads);
                assert(index == real_index && "RACE CONDITION was witnessed");
                /*

                tID ----- th_indices --------- min_times -------- ...
                0   --       30        --       0.34       --
                ------------------------------------------------
                1   --       20        --       0.20       --
                ------------------------------------------------
                ...   --                --                  --
                ------------------------------------------------
                7   --       50        --       0.66       --

                */

                // printf("\ntid acquisitions min_time max_time avg_time\n");
                // for (int i = 0; i < n_threads; i++)
                //     printf("%d %d %f %f %f\n", i, th_indices[i], min_times[i], max_times[i], avg_times[i]);

                /*

                0 3 2 1 2 1 1 1 3 0 ....

                */

                for (int i = 0; i < inner_iterations; i++)
                    printf("%d ", lock_log[i]);
                printf("\n");
            }
        }
        omp_destroy_lock(&baseline);
    }
    return 0;
}