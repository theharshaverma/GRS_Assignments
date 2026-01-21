#!/usr/bin/env python3
"""
MT25024 Part D plotting script
Reads:  MT25024_Part_D_CSV.csv
Writes: MT25024_Part_D_CPU_<workload>.png/.pdf
        MT25024_Part_D_MEM_<workload>.png/.pdf
        MT25024_Part_D_IO_<workload>.png/.pdf
"""

import csv
from collections import defaultdict
from pathlib import Path
import matplotlib.pyplot as plt

CSV_FILE = Path("MT25024_Part_D_CSV.csv")

# data[metric][workload][program] -> list of (count, value)
data = {
    "CPU%": defaultdict(lambda: defaultdict(list)),
    "Mem(KB)": defaultdict(lambda: defaultdict(list)),
    "IO(%util)": defaultdict(lambda: defaultdict(list)),
}

if not CSV_FILE.exists():
    raise FileNotFoundError(f"CSV not found: {CSV_FILE.resolve()}")

with CSV_FILE.open(newline="") as f:
    r = csv.DictReader(f)
    required = {"Program", "Workload", "Count", "CPU%", "Mem(KB)", "IO(%util)"}
    missing = required - set(r.fieldnames or [])
    if missing:
        raise ValueError(f"CSV missing columns: {sorted(missing)}")

    for row in r:
        prog = row["Program"].strip()
        work = row["Workload"].strip()
        count = int(row["Count"])

        data["CPU%"][work][prog].append((count, float(row["CPU%"])))
        data["Mem(KB)"][work][prog].append((count, float(row["Mem(KB)"])))
        data["IO(%util)"][work][prog].append((count, float(row["IO(%util)"])))

def plot_metric(metric_name: str, out_prefix: str, ylabel: str, workloads=("cpu", "mem", "io")):
    for work in workloads:
        plt.figure()

        # plot program_a and program_b if present
        for prog in ("program_a", "program_b"):
            pts = data[metric_name][work].get(prog, [])
            if not pts:
                continue
            pts = sorted(pts, key=lambda x: x[0])
            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            plt.plot(xs, ys, marker="o", label=prog)

        plt.xlabel("Count (processes for program_a, threads for program_b)")
        plt.ylabel(ylabel)
        plt.title(f"{metric_name} vs Count ({work} workload)")
        plt.legend()
        plt.tight_layout()

        png = f"{out_prefix}_{work}.png"
        pdf = f"{out_prefix}_{work}.pdf"
        plt.savefig(png)
        plt.savefig(pdf)
        plt.close()
        print(f"Wrote {png}, {pdf}")

# CPU%
plot_metric("CPU%", "MT25024_Part_D_CPU", "CPU (%)")

# Mem(KB) -> plot in MB for readability
# (still derived from Mem(KB) column)
for work in ("cpu", "mem", "io"):
    # convert to MB on-the-fly
    for prog in list(data["Mem(KB)"][work].keys()):
        data["Mem(KB)"][work][prog] = [(c, kb / 1024.0) for (c, kb) in data["Mem(KB)"][work][prog]]
plot_metric("Mem(KB)", "MT25024_Part_D_MEM", "Memory (MB)")

# IO(%util)
plot_metric("IO(%util)", "MT25024_Part_D_IO", "Disk Utilization (%util)")

print("Done.")
