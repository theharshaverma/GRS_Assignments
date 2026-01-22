# GRS_PA01 - Processes and Threads
### Course: Graduate Systems (CSE638)
### Assignment: PA01 – Processes and Threads
### Name: Harsha Verma
### Roll Number: MT25024

## Overview
This repository contains the complete implementation and analysis for PA01: Processes and Threads. The assignment compares the behavior of processes and threads under CPU-intensive memory-intensive, and I/O-intensive workloads, using automated measurement and analysis.

## Repository Contents
### Source Code
- MT25024_Part_A_Program_A.c – Program A (process-based using fork())
- MT25024_Part_A_Program_B.c – Program B (thread-based using pthread)
- MT25024_Part_B_Workers.c – Worker functions: cpu, mem, io
### Build
- Makefile – Builds Program A and Program B
### Automation Scripts
- MT25024_Part_C_shell.sh – Automation for Part C
- MT25024_Part_D_shell.sh – Automation for Part D (scaling)
- MT25024_Part_D_Plot.py – Plot generation from CSV data
### Measurement Data
- MT25024_Part_C_CSV.csv – Measurements for Part C
- MT25024_Part_D_CSV.csv – Measurements for Part D
### Plots
- MT25024_Part_D_CPU_*.pdf
- MT25024_Part_D_MEM_*.pdf
- MT25024_Part_D_IO_*.pdf
### Report
MT25024_Report.pdf – Final report with screenshots, plots, and analysis

## How to Build
make.
This generates program_a and program_b. Compiled binaries are not committed to the repository as per assignment rules.

## How to Run
### Part C
./MT25024_Part_C_shell.sh
This script:
- Runs all six combinations (A/B × cpu/mem/io)
- Collects CPU, memory, and I/O statistics
- Generates summarized CSV output
### Part D
./MT25024_Part_D_shell.sh
This script:
- Takes user input for the number of child processes and threads
(I have taken 2-5 child process in report and 2-8 for threads)
- Automates execution and measurement for the specified ranges
- Generates CSV files and plots for analysis

## Experimental Setup
- Single-core execution enforced using 'taskset'.
- CPU and memory usage collected using 'top'
- Disk I/O statistics collected using 'iostat'
- Execution time measured using the 'time' command
- All program executions and measurements automated using shell scripts

### AI Usage Declaration
I tools were used for assistance in understanding concepts and refining explanations. All code was written, reviewed, and fully understood.Experimental execution and analysis were performed manually.

### Notes
- No binaries or object files are committed (enforced via .gitignore)
- File naming conventions strictly follow assignment guidelines
- Repository is public as required