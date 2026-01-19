#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "MT25024_Part_B_Workers.h"

int main(int argc, char *argv[]) {
    // 1. Check if the user provided the worker type (cpu, mem, or io)
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cpu|mem|io> [num_processes]\n", argv[0]);
        return 1;
    }

    char *worker_type = argv[1];
    int num_processes = 2; // Default to 2 processes as per Part A [cite: 14]
    
    // Allow changing number of processes for Part D (Scaling)
    if (argc > 2) {
        num_processes = atoi(argv[2]);
    }

    printf("Starting Program A: Creating %d child processes for '%s' task...\n", num_processes, worker_type);

    // 2. Loop to create the exact number of child processes
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            // --- CHILD PROCESS ---
            // The child decides which function to run based on the argument
            if (strcmp(worker_type, "cpu") == 0) {
                cpu_bound(LOOP_COUNT);
            } else if (strcmp(worker_type, "mem") == 0) {
                memory_bound(LOOP_COUNT);
            } else if (strcmp(worker_type, "io") == 0) {
                io_bound(LOOP_COUNT);
            } else {
                // Handle invalid input safely
                fprintf(stderr, "Unknown worker type: %s\n", worker_type);
                exit(1);
            }
            // CRITICAL: Child must exit here, or it will continue the loop and fork more children!
            exit(0); 
        }
        // --- PARENT PROCESS ---
        // Parent does nothing here; it just loops back to fork the next child.
    }

    // 3. Parent waits for ALL child processes to finish
    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }

    printf("Program A: All children finished.\n");
    return 0;
}