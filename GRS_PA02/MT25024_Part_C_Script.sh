#!/usr/bin/env bash
# Roll No- MT25024
# GRS PA02 - Part C: automated runs + perf + CSV
# AI USAGE DECLARATION – MT25024_Part_C.sh (PA02, Graduate Systems)
#
# AI tools (ChatGPT) were used as a supportive aid for Part C in the following ways:
# - Designing an automated experimental workflow to repeatedly run A1, A2, and A3
#   implementations across multiple message sizes and thread counts
# - Structuring a Bash script to set up and tear down Linux network namespaces
#   (ns_c and ns_s) and veth interfaces in an idempotent and reproducible manner
# - Automating client–server execution with warm-up runs followed by perf stat
#   profiling to reduce cold-start measurement noise
# - Parsing application-level output (throughput and RTT) using awk and shell tools
# - Parsing perf stat output on hybrid CPUs by aggregating cpu_core and cpu_atom
#   counters for cpu-cycles and cache-misses
# - Generating a consolidated CSV file suitable for plotting and further analysis
#
# Representative prompts used include:
# - "Write a bash script to automate perf stat experiments across parameters"
# - "How to parse perf stat output on hybrid CPUs (cpu_core / cpu_atom)"
# - "How to aggregate throughput and RTT from multi-threaded client output"
# - "How to safely manage Linux network namespaces in shell scripts"
#
# All script logic was reviewed, understood, and adapted to match the exact
# PA02 setup, binary names, argument formats, and output structure.
# The final script was tested and validated manually on the target machine.

set -euo pipefail

############################
# Config
############################
SERVER_IP="10.200.1.1"
PORT="8989"

# >= 4 message sizes + >= 4 thread counts (edit if needed)
MSG_SIZES=(8192 16384 32768 65536)
THREADS=(4 6 8 12)

DUR=10
WARMUP=2

# Match your manual perf runs exactly
EVENTS="cpu-cycles,cache-misses,context-switches"

OUTDIR="results"
CSV="${OUTDIR}/MT25024_partC_results.csv"

PARTS=(1 2 3)   # A1, A2, A3

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

  # start in background inside ns_s, echo PID
  sudo ip netns exec ns_s bash -lc "./${bin} ${msg} > /dev/null 2>&1 & echo \$!"
}

stop_server() {
  local pid="$1"
  sudo ip netns exec ns_s kill -9 "$pid" >/dev/null 2>&1 || true
}

############################
# Parsing helpers
############################

# Aggregates across all thread lines:
# - total_rx_bytes (sum)
# - aggregate throughput = sum of per-thread throughputs
# - avg_rtt = average of per-thread avg_rtt
# - max_rtt = max across threads
# - time_sec = (last seen) time=...
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

# Parse perf stat output:
# Sum cpu_atom + cpu_core values for cpu-cycles and cache-misses.
# context-switches appears as a single line (sum anyway).
parse_perf() {
  local f="$1"
  awk '
    function n(s){ gsub(/,/,"",s); return s+0; }
    BEGIN{cycles=0; misses=0; cs=0;}
    /cpu-cycles\// { cycles += n($1); }
    /cache-misses\// { misses += n($1); }
    /context-switches/ { cs += n($1); }
    END{ printf "%.0f,%.0f,%.0f\n", cycles, misses, cs; }
  ' "$f"
}

############################
# Main
############################
mkdir -p "$OUTDIR"

echo "part,msg_size,threads,duration_sec,total_rx_bytes,agg_throughput_gbps,avg_rtt_us,max_rtt_us,time_sec,total_cpu_cycles,total_cache_misses,context_switches" > "$CSV"

echo "[INFO] Build..."
make clean >/dev/null
make -j >/dev/null

echo "[INFO] Setup namespaces..."
setup_netns

for p in "${PARTS[@]}"; do
  for m in "${MSG_SIZES[@]}"; do
    for t in "${THREADS[@]}"; do
      tag="A${p}_msg${m}_th${t}_dur${DUR}"
      echo "[RUN] $tag"

      spid="$(start_server "$p" "$m")"
      sleep 1

      # Warm-up (no perf)
      sudo ip netns exec ns_c "./a${p}_client" "$SERVER_IP" "$PORT" "$m" "$t" "$WARMUP" \
        > "${OUTDIR}/warm_${tag}.log" 2>&1 || true

      # Perf run
      app_log="${OUTDIR}/app_${tag}.log"
      perf_log="${OUTDIR}/perf_${tag}.txt"

      sudo ip netns exec ns_c perf stat -e "$EVENTS" "./a${p}_client" "$SERVER_IP" "$PORT" "$m" "$t" "$DUR" \
        > "$app_log" 2> "$perf_log"

      stop_server "$spid"

      IFS=',' read -r total_rx agg_thr avg_rtt max_rtt time_sec < <(parse_client "$app_log")
      IFS=',' read -r cycles misses ctxsw < <(parse_perf "$perf_log")

      echo "${p},${m},${t},${DUR},${total_rx},${agg_thr},${avg_rtt},${max_rtt},${time_sec},${cycles},${misses},${ctxsw}" >> "$CSV"
    done
  done
done

echo "[DONE] CSV written to: $CSV"
echo "[DONE] Logs in: $OUTDIR/"
echo "[INFO] Cleanup namespaces..."
cleanup_netns