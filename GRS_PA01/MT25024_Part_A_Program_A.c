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
    int num_processes = 2; // Default to 2 child processes as per Part A

    // Allow changing number of processes for Part D (Scaling)
    if (argc > 2) {
        num_processes = atoi(argv[2]);
    }

    // Store child PIDs
    pid_t pids[num_processes];

    printf("Starting Program A: Creating %d child processes for '%s' task...\n",
           num_processes, worker_type);

    // 2. Create exactly num_processes children
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            // --- CHILD PROCESS ---
            if (strcmp(worker_type, "cpu") == 0) {
                cpu(LOOP_COUNT);
            } else if (strcmp(worker_type, "mem") == 0) {
                mem(LOOP_COUNT);
            } else if (strcmp(worker_type, "io") == 0) {
                io(LOOP_COUNT);
            } else {
                fprintf(stderr, "Unknown worker type: %s\n", worker_type);
                exit(1);
            }

            // Child must exit to avoid re-forking
            exit(0);
        } else {
            // --- PARENT PROCESS ---
            pids[i] = pid;  // store child PID
        }
    }

    // 3. Parent waits for each specific child
    for (int i = 0; i < num_processes; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("Program A: All children finished.\n");
    return 0;
}
