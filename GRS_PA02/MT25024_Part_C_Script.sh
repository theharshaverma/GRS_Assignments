# ============================================================
# AI USAGE DECLARATION – MT25024_Part_C_Script.sh
# Course: Graduate Systems (CSE638)
# Assignment: PA02 – Network I/O
# Roll No: MT25024
#
# AI tools (ChatGPT) were used as a *supportive aid* for this script in the
# following limited and allowed ways:
#
# - Clarifying the assignment requirements for Part B and Part C
#   (number of runs, parameter variations, and automation expectations)
# - Assisting in structuring a Bash script to automate experiments across
#   multiple message sizes and thread counts
# - Understanding how to correctly run `perf stat` on a server process
#   executing inside a Linux network namespace (ns_s)
# - Helping design safe setup and cleanup procedures for Linux network
#   namespaces and veth interfaces
# - Assisting with parsing application-level output (throughput and RTT)
#   and perf stat counters using awk and shell utilities
#
# Representative prompts used include:
# - "How to run perf stat on a process inside another network namespace"
# - "How to automate experiments across message sizes and thread counts"
# - "How to parse perf stat output for cycles and cache misses"
# - "How to structure a Bash script for reproducible benchmarking"
#
# All experimental design decisions, parameter selections, interpretation
# of results, and final integration were performed independently by the student.
# The script was reviewed, modified, and fully understood before submission.
#
# ============================================================
#!/usr/bin/env bash
# Roll No- MT25024
# GRS PA02 - Part C: automated runs + perf (SERVER namespace) + CSV
# 8 runs per part:
#   - 4 runs: msg sizes (8192,16384,32768,65536) with threads=4
#   - 4 runs: threads (6,8,10,12) with msg=65536

set -euo pipefail

############################
# Config
############################
SERVER_IP="10.200.1.1"
PORT="8989"

DUR=10
WARMUP=2

# perf must run in SERVER namespace (ns_s)
EVENTS="cycles,context-switches,L1-dcache-load-misses,LLC-load-misses"

OUTDIR="results"
CSV="${OUTDIR}/MT25024_partC_results_variation_each_part.csv"

# A1, A2, A3
PARTS=(1 2 3)

# Variation 1: vary message sizes, threads fixed at 4
V1_THREADS=4
V1_MSG_SIZES=(8192 16384 32768 65536)

# Variation 2: vary threads, msg fixed at 65536
V2_MSG_SIZE=65536
V2_THREADS=(6 8 10 12)

############################
# Netns helpers
############################
cleanup_netns() {
  sudo ip netns del ns_s 2>/dev/null || true
  sudo ip netns del ns_c 2>/dev/null || true
  sudo ip link del veth_s 2>/dev/null || true
  sudo ip link del veth_c 2>/dev/null || true
}

setup_netns() {
  cleanup_netns

  sudo ip netns add ns_s
  sudo ip netns add ns_c

  sudo ip link add veth_s type veth peer name veth_c
  sudo ip link set veth_s netns ns_s
  sudo ip link set veth_c netns ns_c

  sudo ip netns exec ns_s ip addr add 10.200.1.1/24 dev veth_s
  sudo ip netns exec ns_c ip addr add 10.200.1.2/24 dev veth_c

  sudo ip netns exec ns_s ip link set lo up
  sudo ip netns exec ns_c ip link set lo up
  sudo ip netns exec ns_s ip link set veth_s up
  sudo ip netns exec ns_c ip link set veth_c up

  sudo ip netns exec ns_c ping -c 1 -W 1 10.200.1.1 >/dev/null
}

start_server() {
  local part="$1"
  local msg="$2"
  local bin="a${part}_server"
  sudo ip netns exec ns_s bash -lc "./${bin} ${msg} > /dev/null 2>&1 & echo \$!"
}

stop_server() {
  local pid="$1"
  sudo ip netns exec ns_s kill -9 "$pid" >/dev/null 2>&1 || true
}

############################
# Parsing helpers
############################
parse_client() {
  local f="$1"
  awk '
    BEGIN{sum_rx=0; sum_thr=0; sum_avg=0; cnt=0; max_max=0; time=""; }
    /\[A[123] client thread\]/{
      if (match($0, /rx_bytes=([0-9]+)/, a)) sum_rx += a[1];
      if (match($0, /rx_throughput=([0-9.]+)/, a)) sum_thr += a[1];
      if (match($0, /avg_rtt=([0-9.]+)/, a)) { sum_avg += a[1]; cnt++; }
      if (match($0, /max_rtt=([0-9.]+)/, a)) if (a[1] > max_max) max_max = a[1];
      if (match($0, /time=([0-9.]+)/, a)) time = a[1];
    }
    END{
      avg_avg = (cnt>0 ? sum_avg/cnt : "");
      printf "%.0f,%.3f,%.3f,%.3f,%s\n", sum_rx, sum_thr, avg_avg, max_max, time;
    }
  ' "$f"
}

parse_perf() {
  local f="$1"
  awk '
    function n(s){ gsub(/,/,"",s); return s+0; }
    BEGIN{cycles=0; cs=0; l1m=0; llcm=0;}

    # Match: cycles OR cpu_core/cycles/ OR cpu_atom/cycles/
    /(\/|[[:space:]])cycles(\/|[[:space:]]|$)/              { cycles += n($1); }

    /context-switches/                                      { cs     += n($1); }
    /L1-dcache-load-misses/                                 { l1m    += n($1); }
    /LLC-load-misses/                                       { llcm   += n($1); }

    END{ printf "%.0f,%.0f,%.0f,%.0f\n", cycles, l1m, llcm, cs; }
  ' "$f"
}

############################
# One run helper
############################
do_one_run() {
  local part="$1"
  local msg="$2"
  local thr="$3"
  local variant="$4"   # "V1" or "V2"
  local tag="A${part}_${variant}_msg${msg}_th${thr}_dur${DUR}"

  printf "\n[RUN] %s\n" "$tag"

  local spid
  spid="$(start_server "$part" "$msg")"
  sleep 1

  # Warm-up (no perf)
  sudo ip netns exec ns_c "./a${part}_client" "$SERVER_IP" "$PORT" "$msg" "$thr" "$WARMUP" \
    > "${OUTDIR}/warm_${tag}.log" 2>&1 || true

  local app_log="${OUTDIR}/app_${tag}.log"
  local perf_log="${OUTDIR}/perf_server_${tag}.txt"

  # perf on server PID in ns_s
  sudo ip netns exec ns_s perf stat -e "$EVENTS" -p "$spid" -- sleep "$DUR" \
    > /dev/null 2> "$perf_log" &
  local perf_pid=$!

  # client run in ns_c
  sudo ip netns exec ns_c "./a${part}_client" "$SERVER_IP" "$PORT" "$msg" "$thr" "$DUR" \
    > "$app_log" 2>&1 || true

  wait "$perf_pid" 2>/dev/null || true
  stop_server "$spid"

  local total_rx agg_thr avg_rtt max_rtt time_sec
  local cycles l1m llcm ctxsw

  IFS=',' read -r total_rx agg_thr avg_rtt max_rtt time_sec < <(parse_client "$app_log")
  IFS=',' read -r cycles l1m llcm ctxsw < <(parse_perf "$perf_log")

  echo "${part},${variant},${msg},${thr},${DUR},${total_rx},${agg_thr},${avg_rtt},${max_rtt},${time_sec},${cycles},${l1m},${llcm},${ctxsw}" >> "$CSV"
}

############################
# Main
############################
mkdir -p "$OUTDIR"

echo "part,variant,msg_size,threads,duration_sec,total_rx_bytes,agg_throughput_gbps,avg_rtt_us,max_rtt_us,time_sec,server_cycles,server_L1_dcache_load_misses,server_LLC_load_misses,server_context_switches" > "$CSV"

printf "[INFO] Build...\n"
make clean >/dev/null
make -j >/dev/null

printf "[INFO] Setup namespaces...\n"
setup_netns

for p in "${PARTS[@]}"; do
  printf "\n========== PART A%d : 8 runs ==========\n" "$p"

  # Variation 1: msg sizes with threads=4
  printf "[INFO] Variation 1 (threads=%d, msg in {8192,16384,32768,65536})\n" "$V1_THREADS"
  for m in "${V1_MSG_SIZES[@]}"; do
    do_one_run "$p" "$m" "$V1_THREADS" "V1"
  done

  # Variation 2: msg=65536 with threads in {6,8,10,12}
  printf "\n[INFO] Variation 2 (msg=%d, threads in {6,8,10,12})\n" "$V2_MSG_SIZE"
  for t in "${V2_THREADS[@]}"; do
    do_one_run "$p" "$V2_MSG_SIZE" "$t" "V2"
  done
done

printf "\n[DONE] CSV written to: %s\n" "$CSV"
printf "[DONE] Logs in: %s/\n" "$OUTDIR"
printf "[INFO] Cleanup namespaces...\n"
cleanup_netns
