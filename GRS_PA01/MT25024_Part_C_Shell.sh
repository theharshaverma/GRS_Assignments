#!/bin/bash

# Compile first
make clean
make

# Define the programs and workloads
PROGRAMS=("program_a" "program_b")
WORKLOADS=("cpu" "mem" "io")
CSV_FILE="MT25024_Part_C_CSV.csv"

# Create CSV Header
echo "Program,Workload,ExecutionTime(s)" > $CSV_FILE
echo "Starting measurements..."

for prog in "${PROGRAMS[@]}"; do
    for work in "${WORKLOADS[@]}"; do
        echo "------------------------------------------------"
        echo "Running: ./$prog $work"

        # 1. Start the program in the background to capture PID
        # We assume 2 workers (default)
        ./$prog $work & 
        PID=$!

        # 2. Run top in batch mode to capture CPU usage of this PID
        # -b: Batch mode, -n 1: One iteration, -p: Process ID
        top -b -n 1 -p $PID > "top_${prog}_${work}.txt" &

        # 3. Wait for the program to finish and capture time
        # Using built-in 'time' is tricky inside scripts, so we use start/end seconds
        START=$(date +%s.%N)
        wait $PID
        END=$(date +%s.%N)

        # Calculate duration
        DURATION=$(echo "$END - $START" | bc)
        echo "Finished $prog $work in $DURATION seconds"

        # Save simple stats to CSV
        echo "$prog,$work,$DURATION" >> $CSV_FILE
    done
done

echo "Done! Results saved to $CSV_FILE"
