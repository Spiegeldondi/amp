#include <stdio.h>
#include <omp.h>

int main() {
    int shared_var = 0;
    #pragma omp parallel num_threads(8)
    {
        int tid = omp_get_thread_num();
        shared_var++; // Increment shared variable without synchronization
        printf("Thread %d updated shared_var to %d\n", tid, shared_var);
    }
    printf("Final value of shared_var: %d\n", shared_var);
    return 0;
}
