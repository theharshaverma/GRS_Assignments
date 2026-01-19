#!/bin/bash
################################################################################
# Assignment: GRS_PA01
# Name: Harsha Verma
# Roll Number: MT25024
# Description: Part C Automation with taskset, iostat, and top metrics
################################################################################

# Compile first using Makefile
make clean
make

# Define programs and workloads
PROGRAMS=("program_a" "program_b")
WORKLOADS=("cpu" "mem" "io")
CSV_FILE="MT25024_Part_C_CSV.csv"

# Create CSV Header with columns: Program+Function, CPU%, Mem, IO
echo "Program+Function,ExecutionTime(s),CPU%,Mem(KB),IO_Wait" > $CSV_FILE

echo "Starting measurements for Part C..."

for prog in "${PROGRAMS[@]}"; do
    for work in "${WORKLOADS[@]}"; do
        echo "------------------------------------------------"
        echo "Running: ./$prog $work"

        # 1. Use taskset to pin to CPU 0 as required
        # 2. Measure execution time using 'time'
        START=$(date +%s.%N)
        
        # Start program in background pinned to CPU 0
        taskset -c 0 ./$prog $work & 
        PID=$!

        # 3. Use iostat to observe disk statistics
        iostat -dx 1 2 > "iostat_${prog}_${work}.txt" &

        # 4. Use top to record CPU% and Memory
        # We capture one batch of the specific PID
        top -b -n 1 -p $PID | grep "$PID" > "top_raw_${prog}_${work}.txt"

        # Wait for the program to finish
        wait $PID
        END=$(date +%s.%N)

        # Data Extraction
        DURATION=$(echo "$END - $START" | bc)
        
        # Extract CPU% (9th col) and Mem (6th col - RES) from top output
        CPU_USAGE=$(awk '{print $9}' "top_raw_${prog}_${work}.txt")
        MEM_USAGE=$(awk '{print $6}' "top_raw_${prog}_${work}.txt")
        
        # Extract Avg IO Wait from iostat (look for 'iowait' in the avg-cpu row)
        IO_WAIT=$(grep -A 1 "avg-cpu" "iostat_${prog}_${work}.txt" | tail -n 1 | awk '{print $4}')

        echo "Finished: $prog+$work | Time: $DURATION | CPU: $CPU_USAGE% | Mem: $MEM_USAGE | IO: $IO_WAIT"

        # Save to CSV
        echo "$prog+$work,$DURATION,$CPU_USAGE,$MEM_USAGE,$IO_WAIT" >> $CSV_FILE
    done
done

# Cleanup temporary raw files
rm top_raw_*.txt
echo "------------------------------------------------"
echo "Done! Part C results saved to $CSV_FILE"