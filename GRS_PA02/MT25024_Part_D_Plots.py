# MT25024
# Part D: Plotting and Visualization (PA02 - GRS)

import matplotlib.pyplot as plt

# =========================
# System Configuration
# =========================
SYSTEM_INFO = "Intel Hybrid CPU, Linux, veth namespaces, TCP"

# =========================
# 1. Throughput vs Message Size
# =========================
msg_sizes = [8192, 16384, 32768, 65536]   # bytes

throughput_A1 = [28.041, 54.772, 90.410, 113.913]   # Gbps (example)
throughput_A2 = [27.094, 48.552, 78.793, 116.512]
throughput_A3 = [21.180, 38.443, 68.331, 101.586]

plt.figure()
plt.plot(msg_sizes, throughput_A1, marker='o', label="A1 Two-Copy")
plt.plot(msg_sizes, throughput_A2, marker='o', label="A2 One-Copy")
plt.plot(msg_sizes, throughput_A3, marker='o', label="A3 Zero-Copy")

plt.xlabel("Message Size (bytes)")
plt.ylabel("Throughput (Gbps)")
plt.title("Throughput vs Message Size\n" + SYSTEM_INFO)
plt.legend()
plt.grid(True)
plt.show()

# =========================
# 2. Latency vs Thread Count
# =========================
threads = [6, 8, 10, 12]

latency_A1 = [23.325, 36.178, 26.328, 24.758]  # microseconds
latency_A2 = [21.505, 30.694, 22.921, 21.515]
latency_A3 = [25.585, 35.924, 30.589, 30.628]

plt.figure()
plt.plot(threads, latency_A1, marker='o', label="A1 Two-Copy")
plt.plot(threads, latency_A2, marker='o', label="A2 One-Copy")
plt.plot(threads, latency_A3, marker='o', label="A3 Zero-Copy")

plt.xlabel("Thread Count")
plt.ylabel("Average Latency (µs)")
plt.title("Latency vs Thread Count\n" + SYSTEM_INFO)
plt.legend()
plt.grid(True)
plt.show()

# =========================
# 3. Cache Misses vs Message Size
# =========================
llc_misses_A1 = [323536, 43679, 93950, 97979]
llc_misses_A2 = [9298, 6242, 15877, 13247]
llc_misses_A3 = [5993, 9351, 12508, 247278]

plt.figure()
plt.plot(msg_sizes, llc_misses_A1, marker='o', label="A1 Two-Copy")
plt.plot(msg_sizes, llc_misses_A2, marker='o', label="A2 One-Copy")
plt.plot(msg_sizes, llc_misses_A3, marker='o', label="A3 Zero-Copy")

plt.xlabel("Message Size (bytes)")
plt.ylabel("LLC Cache Misses")
plt.title("Cache Misses vs Message Size\n" + SYSTEM_INFO)
plt.legend()
plt.grid(True)
plt.show()

# =========================
# 3b. L1 Cache Misses vs Message Size
# =========================
# L1-dcache-load-misses (cpu_core only, as supported on this machine)

l1_misses_A1 = [2819867906, 4174265309, 6180339402, 6365607620]
l1_misses_A2 = [2228314151, 2943118778, 3429221414, 3946730352]
l1_misses_A3 = [1808863232, 2516769069, 3165719655, 3644168300]

plt.figure()
plt.plot(msg_sizes, l1_misses_A1, marker='o', label="A1 Two-Copy")
plt.plot(msg_sizes, l1_misses_A2, marker='o', label="A2 One-Copy")
plt.plot(msg_sizes, l1_misses_A3, marker='o', label="A3 Zero-Copy")

plt.xlabel("Message Size (bytes)")
plt.ylabel("L1 D-cache Load Misses")
plt.title("L1 Cache Misses vs Message Size\n" + SYSTEM_INFO)
plt.legend()
plt.grid(True)
plt.show()

# =========================
# 4. CPU Cycles per Byte
# =========================
cpu_cycles_A1 = [5.365, 2.782, 1.772, 1.221]
cpu_cycles_A2 = [4.883, 2.725, 1.635, 1.098]
cpu_cycles_A3 = [6.589, 3.669, 2.134, 1.435]

plt.figure()
plt.plot(msg_sizes, cpu_cycles_A1, marker='o', label="A1 Two-Copy")
plt.plot(msg_sizes, cpu_cycles_A2, marker='o', label="A2 One-Copy")
plt.plot(msg_sizes, cpu_cycles_A3, marker='o', label="A3 Zero-Copy")

plt.xlabel("Message Size (bytes)")
plt.ylabel("CPU Cycles per Byte")
plt.title("CPU Cycles per Byte Transferred\n" + SYSTEM_INFO)
plt.legend()
plt.grid(True)
plt.show()
