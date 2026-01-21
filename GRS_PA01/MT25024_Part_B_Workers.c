#include "MT25024_Part_B_Workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>

// 1. CPU Task
void cpu(size_t n) {
    for (size_t i = 0; i < n; i++) {
        double result = 0.0;
        for (int j = 0; j < 1000; j++) {
            result += sin(j) * cos(j);
        }
        (void)result; // prevent optimization
    }
}

// 2. Memory Task (allocate once, touch memory repeatedly)
void mem(size_t n) {
    const size_t size = 50UL * 1024UL * 1024UL; // 50MB
    const size_t stride = 64;                  // cache-line stride

    char *buffer = (char *)malloc(size);
    if (!buffer) {
        perror("malloc failed in mem()");
        return;
    }

    // First-touch to ensure pages are mapped
    memset(buffer, 1, size);

    for (size_t iter = 0; iter < n; iter++) {
        for (size_t off = 0; off < size; off += stride) {
            buffer[off] ^= (char)(iter & 0xFF);
        }

        // Prevent compiler from optimizing loop away
        volatile char sink = buffer[(iter * 4096) % size];
        (void)sink;
    }

    free(buffer);
}

// 3. IO Task
void io(size_t n) {
    char filename[64];

    // Unique filename per thread
    snprintf(filename, sizeof(filename),
             "io_test_%lu.bin", (unsigned long)pthread_self());

    const size_t BUF_SIZE = 256UL * 1024UL;
    const size_t FSYNC_EVERY = 10;

    char *buf = (char *)malloc(BUF_SIZE);
    if (!buf) {
        perror("malloc failed in io()");
        return;
    }
    memset(buf, 'A', BUF_SIZE);

    for (size_t i = 0; i < n; i++) {
        FILE *fp = fopen(filename, "wb");
        if (!fp) {
            perror("fopen failed in io()");
            break;
        }

        fwrite(buf, 1, BUF_SIZE, fp);

        if ((i % FSYNC_EVERY) == 0) {
            fflush(fp);
            fsync(fileno(fp));
        }

        fclose(fp);
    }

    remove(filename);
    free(buf);
}

