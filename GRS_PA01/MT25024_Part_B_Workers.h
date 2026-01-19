#ifndef WORKERS_H
#define WORKERS_H

#include <stddef.h> 

// ROLL NO IS MT25024 -> Last digit is 4.
// Assignment says: Last digit * 1000.
// So your loop count is 4000.
#define LOOP_COUNT 4000 

void cpu(size_t n);
void mem(size_t n);
void io(size_t n);

#endif
