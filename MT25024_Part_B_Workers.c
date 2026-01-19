#include "MT25024_Part_B_Workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// 1. CPU Task
void cpu_bound(size_t n) {
    for (size_t i = 0; i < n; i++) {
        double result = 0.0;
        for (int j = 0; j < 1000; j++) {
            result += sin(j) * cos(j);
        }
    }
}

// 2. Memory Task
void memory_bound(size_t n) {
    size_t size = 50 * 1024 * 1024; // 50MB
    for (size_t i = 0; i < n; i++) {
        char *buffer = (char *)malloc(size);
        if (buffer) {
            memset(buffer, 0, size);
            free(buffer);
        }
    }
}

// 3. IO Task
void io_bound(size_t n) {
    const char *filename = "io_test_temp.txt";
    const char *data = "Writing data to disk for IO test.\n";
    size_t len = strlen(data);

    for (size_t i = 0; i < n; i++) {
        FILE *fp = fopen(filename, "w");
        if (fp) {
            for (int j = 0; j < 100; j++) {
                fwrite(data, 1, len, fp);
            }
            fsync(fileno(fp)); // Force write to disk
            fclose(fp);
        }
        remove(filename);
    }
}