#!/bin/bash
################################################################################
# Assignment: GRS_PA01
# Name: Harsha Verma
# Roll Number: MT25024
# Description: Part C Automation with taskset, top sampling, iostat
# Output: Program+Function, CPU%, Mem(KB), IO(%util)
################################################################################

set -u

need_cmd() { command -v "$1" >/dev/null 2>&1 || { echo "Error: '$1' not found."; exit 1; }; }

need_cmd make
need_cmd taskset
need_cmd top
need_cmd iostat
need_cmd awk
need_cmd pgrep
need_cmd sed
need_cmd sort
need_cmd paste
need_cmd kill
need_cmd sleep

# -------- Compile --------
make clean
make

PROGRAMS=("program_a" "program_b")
WORKLOADS=("cpu" "mem" "io")
CSV_FILE="MT25024_Part_C_CSV.csv"

echo "Program+Function,CPU%,Mem(KB),IO(%util)" > "$CSV_FILE"
echo "Starting measurements for Part C..."

# Get all descendants of a PID (children, grandchildren, ...)
get_descendants_csv() {
  local root="$1"
  local kids newkids all=""
  kids=$(pgrep -P "$root" 2>/dev/null || true)
  all="$kids"
  while [ -n "$kids" ]; do
    newkids=""
    for k in $kids; do
      newkids="$newkids $(pgrep -P "$k" 2>/dev/null || true)"
    done
    kids=$(echo "$newkids" | xargs -n1 2>/dev/null | sort -u || true)
    all="$all $kids"
  done
  echo "$all" | xargs -n1 2>/dev/null | sort -u | paste -sd, - 2>/dev/null || true
}

# Better sampling (fixes CPU=0.0 for short runs)
WARMUP_SLEEP="0.2"
SAMPLE_SLEEP="0.2"

for prog in "${PROGRAMS[@]}"; do
  for work in "${WORKLOADS[@]}"; do
    echo "------------------------------------------------"
    echo "Running: ./$prog $work"

    # Start iostat disk stats (1s interval)
    iostat -dx 1 > "iostat_${prog}_${work}.txt" 2>/dev/null &
    IOSTAT_PID=$!

    # Run program pinned to CPU0
    taskset -c 0 ./"$prog" "$work" &
    PID=$!

    # allow program to start + spawn children before first sample
    sleep "$WARMUP_SLEEP"

    > "top_log_${prog}_${work}.txt"

    while kill -0 "$PID" 2>/dev/null; do
      DESC=$(get_descendants_csv "$PID")
      if [ -n "$DESC" ]; then
        TOP_PIDS="$PID,$DESC"
      else
        TOP_PIDS="$PID"
      fi

      top -b -n 1 -p "$TOP_PIDS" 2>/dev/null \
        | grep -E '^[[:space:]]*[0-9]+' \
        | awk '
          function to_kb(v,  last, unit, num){
            last=substr(v,length(v),1)
            if(last ~ /[gGmMkK]/){ unit=tolower(last); num=substr(v,1,length(v)-1) }
            else { unit=""; num=v }
            if(num=="") return 0
            if(unit=="g") return num*1024*1024
            if(unit=="m") return num*1024
            if(unit=="k") return num
            return num  # assume KB if no unit
          }
          { cpu+=$9; mem+=to_kb($6) }
          END { printf "%.1f %.0f\n", cpu, mem }
        ' >> "top_log_${prog}_${work}.txt"

      sleep "$SAMPLE_SLEEP"
    done

    wait "$PID" 2>/dev/null

    kill "$IOSTAT_PID" 2>/dev/null
    wait "$IOSTAT_PID" 2>/dev/null

    # Average CPU and Mem over samples
    CPU_USAGE=$(awk '{sum+=$1; n++} END{ if(n) printf "%.1f", sum/n; else print "0.0" }' "top_log_${prog}_${work}.txt")
    MEM_USAGE=$(awk '{sum+=$2; n++} END{ if(n) printf "%.0f", sum/n; else print "0" }' "top_log_${prog}_${work}.txt")

    # IO metric: max %util across devices (sdX, nvme0n1, etc.)
    IO_UTIL=$(awk '
      $1 ~ /^(Device:|Linux|avg-cpu:|)$/ { next }
      $1 ~ /^[A-Za-z0-9._-]+$/ {
        if ($NF+0 > max) max=$NF
      }
      END { print (max=="" ? "0.00" : max) }
    ' "iostat_${prog}_${work}.txt")

    # Print ONLY CPU, Mem, IO
    echo "Finished: $prog+$work | CPU(avg): $CPU_USAGE% | Mem(avg): ${MEM_USAGE}KB | IO(max %util): $IO_UTIL"

    # CSV with ONLY CPU, Mem, IO
    echo "$prog+$work,$CPU_USAGE,$MEM_USAGE,$IO_UTIL" >> "$CSV_FILE"
  done
done

rm -f top_log_*.txt iostat_*.txt
echo "------------------------------------------------"
echo "Done! Part C results saved to $CSV_FILE"
