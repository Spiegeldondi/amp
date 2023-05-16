#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define n_threads 4

void filter_lock(volatile int* level, volatile int* victim, int tid) {
    //volatile int wait;
    for (int j=1; j<n_threads; j++) {
        level[tid] = j;
        victim[j] = tid;
        for (int k=0; k<n_threads; k++) {
            if (k==tid) continue;
            while (level[k]>=j && victim[j]==tid) {}; // works
        }
    }
    //return wait;
}

int sum_val(volatile int* values, int size) {
    int sum = 0;
    for (int i=0; i<size; i++) {sum+=values[i];}
    return sum;
}

void block_woo_lock(volatile int* competing, volatile int* victim, int tid) {
    int j=0;
    competing[tid] = 1;
    do {
        j++;
        victim[j] = tid;
        while (victim[j]==tid && j<sum_val(competing, n_threads)) {};
        // printf("tid: %d victim[%d]: %d sum_val: %d\n", tid, j, victim[j], sum_val(competing, n_threads));
    } while (victim[j] != tid);
    // printf("tid: %d entereing CS at victim[%d]: %d \n", tid, j, victim[j]);
}

void block_woo_unlock(volatile int* competing, int tid) {
    competing[tid] = 0;
}

void filter_unlock(volatile int* level, int tid) {
    level[tid] = 0;
}

double avg_val(double* values, int size) {
    double sum = 0;
    for (int i=0; i<size; i++) {
        sum += values[i];
    }
    return sum / size;
}

double std_dev(double* values, int size) {
    double avg = avg_val(values, size);
    double sum_sq_diff = 0;
    for (int i=0; i<size; i++) {
        double diff = values[i] - avg;
        sum_sq_diff += diff * diff;
    }
    return sqrt(sum_sq_diff / size);
}

int main (int argc, char** argv) {
    volatile int level[n_threads], victim_filter[n_threads],victim_woo[n_threads];
    volatile int competing[n_threads] ={0}; // need to set array to zero for sum_val to work
    int lock_log[n_threads]; // stores order of access into CSQ[i]  by thread_ID
    int tid, index;
    index = 0;

    int shots = 1;
    double timings[shots];
    double timing_avg = 0;

    double start, stop;

    omp_lock_t baseline;
    omp_init_lock(&baseline);

for (int s=0; s<shots; s++) {
    // BASELINES RUN
    index = 0;
    #pragma omp parallel private(tid) shared(level, victim_filter, victim_woo, competing, lock_log, index)
    {
        tid = omp_get_thread_num();

        start = omp_get_wtime();
        //omp_set_lock(&baseline);
        //filter_lock(level, victim_filter, tid);
        block_woo_lock(competing, victim_woo, tid);

        // Critical section //////////
        lock_log[index] = tid;
        index += 1;
        //////////////////////////////
        
        // omp_unset_lock(&baseline);
        // filter_unlock(level, tid);
        block_woo_unlock(competing, tid);
        stop = omp_get_wtime();
    }
    timings[s] = ((double)stop-start);
}
    omp_destroy_lock(&baseline);

    // check for correctness
    // in case of race condition, index will likely be < n_threads
    assert(index==n_threads && "RACE CONDITION was witnessed");

    // analyze order of access into cs
    printf("access order:\n");
    for (int n=0; n<n_threads; n++) {
        printf("%d ", lock_log[n]);
    }
    printf("\n");

    printf("time: \n%.2f +/- %.2f\n", 10e6 * avg_val(timings, shots), 10e6 * std_dev(timings, shots));

    return 0;
}    