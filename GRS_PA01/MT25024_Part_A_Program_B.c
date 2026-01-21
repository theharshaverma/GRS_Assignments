#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <string.h>
#include "MT25024_Part_B_Workers.h"

// Thread wrapper function
void *thread_wrapper(void *arg) {
    char *worker_type = (char *)arg;

    if (strcmp(worker_type, "cpu") == 0) {
        cpu(LOOP_COUNT);
    } else if (strcmp(worker_type, "mem") == 0) {
        mem(LOOP_COUNT);
    } else if (strcmp(worker_type, "io") == 0) {
        io(LOOP_COUNT);
    } else {
        // Handle invalid input safely (important for demo/viva)
        fprintf(stderr, "Unknown worker type: %s\n", worker_type);
        pthread_exit(NULL);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cpu|mem|io> [num_threads]\n", argv[0]);
        return 1;
    }

    char *worker_type = argv[1];
    int num_threads = 2; // Default to 2 threads 

    if (argc > 2) {
        num_threads = atoi(argv[2]);
    }

    printf("Starting Program B: Creating %d threads for '%s' task...\n", num_threads, worker_type);

    pthread_t threads[num_threads]; 

    for (int i = 0; i < num_threads; i++) {
        int result = pthread_create(&threads[i], NULL, thread_wrapper, (void *)worker_type);
        if (result != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Program B: All threads finished.\n");
    return 0;
}
