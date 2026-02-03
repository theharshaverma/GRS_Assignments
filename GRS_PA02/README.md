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
In Part B, we examine the performance characteristics of the client application through CPU and system-level profiling of CPU message exchange experiments. Because all experiments were run within a VirtualBox virtual machine, direct hardware access to performance monitoring units (PMUs) was not possible. Consequently, profiling was carried out using software counters available through perf stat.

The experiments evaluate application-level throughput and latency, as well as system-level metrics such as task-clock (CPU time), context switches, CPU migrations, and page faults. Throughput is calculated as the total amount of data received by the client (summed across all threads) divided by the runtime in Gbps. Latency is measured as the round-trip time (RTT) per message using clock_gettime(CLOCK_MONOTONIC) at the client.

The profiling was carried out for varying message sizes and thread counts to examine the effect of message granularity and concurrency on performance. The results obtained can be used to interpret scalability trends, including throughput saturation, increased latency, and higher context-switch costs with increasing thread counts.

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
Run the client inside the client namespace without perf :
```bash
sudo ip netns exec ns_c ./a1_client <server_ip> 8989 <msg_size> <threads> <duration_sec>
```
eg:
```bash
sudo ip netns exec ns_c ./a1_client 10.200.1.1 8989 65536 4 10
```
Run the client inside the client namespace with perf :
```bash
sudo ip netns exec ns_c perf stat -e task-clock,context-switches,cpu-migrations,page-faults ./a1_client <server_ip> 8989 <msg_size> <threads> 10
```
eg:
```bash
sudo ip netns exec ns_c perf stat -e task-clock,context-switches,cpu-migrations,page-faults ./a1_client 10.200.1.1 8989 65536 4 10
```