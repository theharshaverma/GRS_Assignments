#!/usr/bin/env bash
# Roll No- MT25024
# GRS PA02 - Part C: automated runs + perf (SERVER namespace) + CSV
# AI USAGE DECLARATION – MT25024_Part_C_Script.sh (PA02, Graduate Systems)
#
# AI tools (ChatGPT) were used as a supportive aid for Part C in the following ways:
# - Designing an automated experimental workflow to repeatedly run A1, A2, and A3
#   implementations across multiple message sizes and thread counts
# - Structuring a Bash script to set up and tear down Linux network namespaces
#   (ns_c and ns_s) and veth interfaces in an idempotent and reproducible manner
# - Automating client–server execution with warm-up runs followed by perf stat
#   profiling to reduce cold-start measurement noise
# - Parsing application-level output (throughput and RTT) using awk and shell tools
# - Parsing perf stat output by summing counters (and handling hybrid CPU formats)
# - Generating a consolidated CSV file suitable for plotting and further analysis
#
# Representative prompts used include:
# - "Write a bash script to automate perf stat experiments across parameters"
# - "How to run perf stat on a server process running in another network namespace"
# - "How to parse perf stat output and extract cycles/context-switches/L1/LLC misses"
# - "How to aggregate throughput and RTT from multi-threaded client output"
# - "How to safely manage Linux network namespaces in shell scripts"
#
# All script logic was reviewed, understood, and adapted to match the exact
# PA02 setup, binary names, argument formats, and output structure.

set -euo pipefail

############################
# Config
############################
SERVER_IP="10.200.1.1"
PORT="8989"

# EXACTLY 4 message sizes
MSG_SIZES=(8192 16384 32768 65536)

# EXACTLY 1 thread count (fixed)
THREADS=(4)

DUR=10
WARMUP=2

# perf must run in SERVER namespace (ns_s)
EVENTS="cycles,context-switches,L1-dcache-load-misses,LLC-load-misses"

OUTDIR="results"

# Write a separate CSV so you don't overwrite your full 48-run CSV
CSV="${OUTDIR}/MT25024_partC_results_8runs.csv"

# EXACTLY 2 parts => 2 * 4 * 1 = 8 total runs
# (A1 + A3). If you want A1 + A2, set PARTS=(1 2).
PARTS=(1 3)

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
# Main
############################
mkdir -p "$OUTDIR"

echo "part,msg_size,threads,duration_sec,total_rx_bytes,agg_throughput_gbps,avg_rtt_us,max_rtt_us,time_sec,server_cycles,server_L1_dcache_load_misses,server_LLC_load_misses,server_context_switches" > "$CSV"

printf "[INFO] Build...\n"
make clean >/dev/null
make -j >/dev/null

printf "[INFO] Setup namespaces...\n"
setup_netns

for p in "${PARTS[@]}"; do
  for m in "${MSG_SIZES[@]}"; do
    for t in "${THREADS[@]}"; do
      tag="A${p}_msg${m}_th${t}_dur${DUR}"
      printf "\n[RUN] %s\n" "$tag"

      spid="$(start_server "$p" "$m")"
      sleep 1

      # Warm-up (no perf)
      sudo ip netns exec ns_c "./a${p}_client" "$SERVER_IP" "$PORT" "$m" "$t" "$WARMUP" \
        > "${OUTDIR}/warm_${tag}.log" 2>&1 || true

      app_log="${OUTDIR}/app_${tag}.log"
      perf_log="${OUTDIR}/perf_server_${tag}.txt"

      # perf on server process in server namespace
      sudo ip netns exec ns_s perf stat -e "$EVENTS" -p "$spid" -- sleep "$DUR" \
        > /dev/null 2> "$perf_log" &
      perf_pid=$!

      # client run in client namespace
      sudo ip netns exec ns_c "./a${p}_client" "$SERVER_IP" "$PORT" "$m" "$t" "$DUR" \
        > "$app_log" 2>&1 || true

      wait "$perf_pid" 2>/dev/null || true
      stop_server "$spid"

      IFS=',' read -r total_rx agg_thr avg_rtt max_rtt time_sec < <(parse_client "$app_log")
      IFS=',' read -r cycles l1m llcm ctxsw < <(parse_perf "$perf_log")

      echo "${p},${m},${t},${DUR},${total_rx},${agg_thr},${avg_rtt},${max_rtt},${time_sec},${cycles},${l1m},${llcm},${ctxsw}" >> "$CSV"
    done
  done
done

printf "\n[DONE] CSV written to: %s\n" "$CSV"
printf "[DONE] Logs in: %s/\n" "$OUTDIR"
printf "[INFO] Cleanup namespaces...\n"
cleanup_netns
