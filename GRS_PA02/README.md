# GRS_PA02 - Network I/O
### Course: Graduate Systems (CSE638)
### Assignment: PA02 – Network I/O
### Name: Harsha Verma
### Roll Number: MT25024

## Part A1
### Overview
This section will implement a TCP-based client-server program with the following characteristics:
- The **server** will be written in a *thread-per-client* manner using `pthread`
- The **client** will send an 8-byte trigger message repeatedly; the **server** will respond to each iteration with a fixed-size message of `msgSize` bytes
- The server-side message will be logically organized as **8 dynamically allocated string members (heap-allocated)**, meeting the assignment requirement
- To optimize performance, the server will **serialize (pack) these 8 members into a single buffer only once per connection**, and then transmit the packed message using `send()` on each trigger
- The client and server programs will execute in **separate Linux network namespaces** to mimic a real-world distributed setting without virtualization
- Part A1 will employ only **`send()`/`recv()`** system calls, as mandated

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
sudo ip netns exec ns_c ./a1_client <server_ip> <port> <msg_size> <threads> <duration_sec>
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
Part A2 extends Part A1 by implementing a structured server-to-client message using **sendmsg()/recvmsg()** with **scatter–gather I/O (iovec)**. Each message consists of **8 dynamically allocated string fields**.

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
sudo ip netns exec ns_c ./a2_client <server_ip> <port> <msg_size> <threads> <duration_sec>
```
eg: 
```bash
sudo ip netns exec ns_c ./a2_client 10.200.1.1 8989 65536 4 10
```
