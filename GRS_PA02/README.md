<!--MT25024 -->
# GRS_PA02 - Network I/O
### Course: Graduate Systems (CSE638)
### Assignment: PA02 – Network I/O
### Name: Harsha Verma
### Roll Number: MT25024

## Part A1
### Overview
This section will implement a TCP-based client-server program with the following characteristics:
- The **server** will be written in a *thread-per-client* manner using `pthread`
- The client sends an 8-byte trigger; for each trigger, the server sends back a fixed-size response of msgSize bytes
- The server message is conceptually divided into 8 dynamically allocated (heap-allocated) string fields, meeting the assignment requirement
- For each trigger, the server packs these 8 heap-allocated fields into a single contiguous buffer in user space and sends it over the network using send(), establishing the two-copy baseline.
- The client and server run in distinct Linux network namespaces to simulate a distributed setting without virtualization
- Part A1 strictly employs only send() and recv() system calls, as required for the baseline implementation

### Network Namespace Setup
client and server are run on separate Linux network namespaces
(ns_c for client, ns_s for server) connected via a veth pair. 
```bash
# Create namespaces
sudo ip netns add ns_s
sudo ip netns add ns_c

# Create veth pair-
sudo ip link add veth_s type veth peer name veth_c

# Move interfaces into namespaces
sudo ip link set veth_s netns ns_s
sudo ip link set veth_c netns ns_c

# Assign IP addresses:
sudo ip netns exec ns_s ip addr add 10.200.1.1/24 dev veth_s
sudo ip netns exec ns_c ip addr add 10.200.1.2/24 dev veth_c

# Bring interfaces up-
sudo ip netns exec ns_s ip link set lo up
sudo ip netns exec ns_c ip link set lo up
sudo ip netns exec ns_s ip link set veth_s up
sudo ip netns exec ns_c ip link set veth_c up

#Verify Connectivity
sudo ip netns exec ns_c ping -c 1 10.200.1.1

#Check existing namespaces
ip netns list
```

### Running the Server (A1)
Running the the server inside the server namespace:
```bash
sudo ip netns exec ns_s ./a1_server <msg_size>
```
eg: 
```bash
sudo ip netns exec ns_s ./a1_server 65536
```

### Running the Client (A1)
Run the client inside the client namespace:
```bash
sudo ip netns exec ns_c ./a1_client <server_ip> 8989 <msg_size> <threads> <duration_sec>
```
eg:
```bash
sudo ip netns exec ns_c ./a1_client 10.200.1.1 8989 65536 4 10
```
Each client thread reports its individual throughput at the end of execution; aggregate throughput is computed by summing throughput across all threads during analysis.

### Delete the namespaces
```bash
sudo ip netns del ns_s 2>/dev/null
sudo ip netns del ns_c 2>/dev/null
sudo ip link del veth_s 2>/dev/null
```

## Part A2
### Overview
Part A2 expands Part A1 by incorporating the server response message through **scatter-gather I/O**:

- The **client sends an 8-byte trigger** repeatedly.
- Each trigger is responded to by the **server with a fixed-size message of `msgSize` bytes**.
- The server’s message is represented by a **structure with 8 dynamically allocated (heap) string buffers/byte arrays**, whose total size is `msgSize`.
- The server sends the message through **`sendmsg()` with 8 `iovec` arguments** (one for each member).
- The client receives the message through **`recvmsg()` with 8 `iovec` arguments** (one for each member).
- The server is **thread-per-client** using `pthread`, and the execution takes place in **separate network namespaces**.

### Running the Server (A2)
```bash
sudo ip netns exec ns_s ./a2_server <msg_size>
```
eg: 
```bash
sudo ip netns exec ns_s ./a2_server 65536
```

### Running the Client (A2)
```bash
sudo ip netns exec ns_c ./a2_client <server_ip> 8989 <msg_size> <threads> <duration_sec>
```
eg: 
```bash
sudo ip netns exec ns_c ./a2_client 10.200.1.1 8989 65536 4 10
```
Each client thread reports its **per-thread receive throughput**; aggregate throughput is the sum across all threads.

## Part A3
### Overview Zero-Copy TCP using `MSG_ZEROCOPY`
Part A3 adds to Part A2 by supporting **kernel-assisted zero-copy transmission on the server → client data path** via Linux TCP `MSG_ZEROCOPY`.

- The **client sends an 8-byte trigger** repeatedly (same as Part A2).
- For each trigger, the **server sends a fixed-size message of `msgSize` bytes**.
- The response message is described as a **structure containing 8 dynamically allocated (heap) buffers**, with a total size of `msgSize`.
- The server sends the response using **`sendmsg()` with 8 `iovec` entries** and the **`MSG_ZEROCOPY` flag**.
- The client receives the response using **`recvmsg()` with 8 `iovec` entries** (same client code as Part A2).
- The server employs a **thread-per-client** approach implemented via `pthread`.
- The test occurs in **separate Linux network namespaces**, as in Parts A1 and A2.

### Running the Server (A3)
```bash
sudo ip netns exec ns_s ./a3_server <msg_size>
```
eg: 
```bash
sudo ip netns exec ns_s ./a3_server 65536
```

### Running the Client (A3)
```bash
sudo ip netns exec ns_c ./a3_client <server_ip> 8989 <msg_size> <threads> <duration_sec>
```
eg: 
```bash
sudo ip netns exec ns_c ./a3_client 10.200.1.1 8989 65536 4 10
```

## Part B
Part B is concerned with profiling and performance analysis of the TCP-based implementations from Parts A1, A2, and A3. All experiments were conducted using Linux network namespaces (`ns_c` for client and `ns_s` for server) on the same machine to isolate the execution of the client and server while still allowing access to hardware performance counters.

Application-level performance metrics include **throughput and latency**, measured at the **client**, while system-level dynamics are analyzed using **`perf stat`**, mainly focused on the **server process** to analyze the CPU and memory subsystem costs of data transmission.

### Application-Level Metrics (Client-Side)
**Throughput** is computed as the total data received by the client, summed across all threads, divided by the total time taken and expressed in Gbps:
Total bytes = sum of rx_bytes (across all client threads)
Throughput (Gbps) = (Total bytes × 8) / (time × 10⁹)

The client threads report their received bytes and the time taken; the throughput values shown in the tables are the **total across all threads**.

**Latency** is measured as the **round-trip time (RTT)** per message at the client:
- A timestamp is taken immediately before sending the 8-byte trigger.
- A second timestamp is taken after the full response is received.
- RTT is computed using `clock_gettime(CLOCK_MONOTONIC)`.

Both **average latency** and **maximum latency** are reported. Average latency reflects steady-state performance, while maximum latency captures tail effects due to OS scheduling, interrupt handling, and kernel TCP processing.

### System-Level Metrics (PMU Profiling)

System-level metrics are collected using `perf stat` while profiling the **server process** running inside `ns_s`. Profiling the server captures the CPU, cache, and scheduling costs of message transmission and kernel-level data movement.

The following metrics are reported:

#### CPU Cycles / CPU Time
CPU utilization is measured using hardware and software counters:
- `task-clock` measures total CPU time consumed by the server process.
- `cpu-cycles` measures hardware execution cycles.

On hybrid CPUs, `perf stat` reports cycles separately for:
- Performance cores (`cpu_core`)
- Efficiency cores (`cpu_atom`)

For reporting aggregate CPU cost: Total CPU Cycles = cpu_core_cycles + cpu_atom_cycles
#### Cache Misses
The cache activity is measured by using cache miss events provided by the platform:

- **L1 Data Cache Misses**  
  `L1-dcache-load-misses` are available **only for cpu_core** on this machine.  
  Hence, L1 cache misses are measured **only for cpu_core**.

- **Last-Level Cache (LLC) Misses**  
  `LLC-load-misses` are available for both cpu_core and cpu_atom.  
  Total LLC misses are calculated as: Total LLC-load-misses = LLC-load-misses(cpu_core) + LLC-load-misses(cpu_atom)

The LLC misses give a very good sense of the memory traffic and cache pressure, and are especially useful when comparing two-copy, one-copy, and zero-copy code.

#### Context Switches and Other Metrics
Context switches are measured via the `context-switches` counter of `perf stat`. A higher number of context switches is an indication of higher scheduling overhead due to higher thread concurrency.

CPU migrations and page faults were also tracked. Page faults are mainly an indication of the initial memory allocation overhead, and there was no memory thrashing in the steady-state execution.
  
### Running the Server 
Running the the server inside the server namespace:
```bash
sudo ip netns exec ns_s ./a<part_no>_server <msg_size>
```
eg: 
```bash
sudo ip netns exec ns_s ./a1_server 65536
```
Run the server inside the client namespace with perf stat profiling enabled:
```bash
sudo ip netns exec ns_s perf stat -p <server_pid> -e \
cycles,context-switches,L1-dcache-load-misses,LLC-load-misses -- sleep <duration_sec>
```
### Running the Client 
Run the client inside the client namespace without perf :
```bash
sudo ip netns exec ns_c ./a<part_no>_client <server_ip> 8989 <msg_size> <threads> <duration_sec>
```
eg:
```bash
sudo ip netns exec ns_c ./a1_client 10.200.1.1 8989 65536 4 10
```
## Part C Automated Experiments and CSV Generation
Part C employs automation for the experimental evaluation of Part A1, A2, and A3 through the application of a shell script: MT25024_Part_C_Script.sh. This script varies the implementation type, message size, as well as the thread count while keeping the execution duration fixed.
For each run:
The server is executed inside ns_s.
The client is executed within ns_c and measures application-level metrics:
- Aggregate throughput
- Average RTT
- Maximum RTT
perf stats is used and it is appended to the server process:
- CPU cycles (cpu_core + cpu_atom)
- L1 d-cache load
- LLC-load-misses(cpu_core + cpu_atom
- Context switches

All results are aggregated and written into a single consolidated CSV file:
MT25024_Part_C_CSV.csv
Each row is an individual experimental configuration. It is immediately useful without change in the tables and sections in Part B of the report. Intermediate log files are not committed because they are unnecessary clutter in the repository; all the results can be reproduced by rerunning the script.
To Reproduce:
```bash
chmod +x MT25024_Part_C_Script.sh
sudo ./MT25024_Part_C_Script.sh
```
