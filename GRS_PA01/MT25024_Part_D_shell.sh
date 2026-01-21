#!/bin/bash
################################################################################
# Assignment: GRS_PA01
# Name: Harsha Verma
# Roll Number: MT25024
# Description: Part D Scaling (incremental, terminal + CSV output)
################################################################################

set -u

need_cmd() { command -v "$1" >/dev/null 2>&1 || { echo "Error: '$1' not found."; exit 1; }; }

need_cmd make
need_cmd taskset
need_cmd top
need_cmd iostat
need_cmd awk
need_cmd pgrep
need_cmd sort
need_cmd paste
need_cmd kill
need_cmd sleep
need_cmd xargs

# ---------------- USER INPUT ----------------
read -p "Program A processes start (e.g., 2): " A_START
read -p "Program A processes end   (e.g., 5): " A_END
read -p "Program B threads start   (e.g., 2): " B_START
read -p "Program B threads end     (e.g., 8): " B_END

# ---------------- Compile ----------------
make clean
make

WORKLOADS=("cpu" "mem" "io")
CSV_FILE="MT25024_Part_D_CSV.csv"

echo "Program,Workload,Count,CPU%,Mem(KB),IO(%util)" > "$CSV_FILE"
echo "Starting Part D measurements..."

# ---------------- Helpers ----------------
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
  echo "$all" | xargs -n1 2>/dev/null | sort -u | paste -sd, -
}

WARMUP_SLEEP=0.2
SAMPLE_SLEEP=0.2

run_one() {
  local prog="$1"
  local work="$2"
  local count="$3"

  echo "------------------------------------------------"
  echo "Running: ./$prog $work $count"

  # Start iostat
  iostat -dx 1 > "iostat_${prog}_${work}_${count}.txt" 2>/dev/null &
  IOSTAT_PID=$!

  # Run program (let program print its own messages)
  taskset -c 0 ./"$prog" "$work" "$count" &
  PID=$!

  sleep "$WARMUP_SLEEP"
  > "top_log_${prog}_${work}_${count}.txt"

  while kill -0 "$PID" 2>/dev/null; do
    DESC=$(get_descendants_csv "$PID")
    [ -n "$DESC" ] && TOP_PIDS="$PID,$DESC" || TOP_PIDS="$PID"

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
          return num
        }
        { cpu+=$9; mem+=to_kb($6) }
        END { printf "%.1f %.0f\n", cpu, mem }
      ' >> "top_log_${prog}_${work}_${count}.txt"

    sleep "$SAMPLE_SLEEP"
  done

  wait "$PID" 2>/dev/null
  kill "$IOSTAT_PID" 2>/dev/null
  wait "$IOSTAT_PID" 2>/dev/null

  CPU=$(awk '{s+=$1;n++} END{printf "%.1f",(n?s/n:0)}' "top_log_${prog}_${work}_${count}.txt")
  MEM=$(awk '{s+=$2;n++} END{printf "%.0f",(n?s/n:0)}' "top_log_${prog}_${work}_${count}.txt")

  IO=$(awk '
    $1 ~ /^(Device:|Linux|avg-cpu:|)$/ { next }
    $1 ~ /^[A-Za-z0-9._-]+$/ { if ($NF+0 > max) max=$NF }
    END { print (max=="" ? "0.00" : max) }
  ' "iostat_${prog}_${work}_${count}.txt")

  # ----- REQUIRED TERMINAL PRINT -----
  echo "Finished: ${prog}+${work}+${count} | CPU(avg): ${CPU}% | Mem(avg): ${MEM}KB | IO(max %util): ${IO}"

  # ----- CSV -----
  echo "${prog},${work},${count},${CPU},${MEM},${IO}" >> "$CSV_FILE"
}

# ---------------- Program A ----------------
for ((n=A_START; n<=A_END; n++)); do
  for w in "${WORKLOADS[@]}"; do
    run_one "program_a" "$w" "$n"
  done
done

# ---------------- Program B ----------------
for ((n=B_START; n<=B_END; n++)); do
  for w in "${WORKLOADS[@]}"; do
    run_one "program_b" "$w" "$n"
  done
done

rm -f top_log_*.txt iostat_*.txt
echo "------------------------------------------------"
echo "Done! Part D results saved to $CSV_FILE"
