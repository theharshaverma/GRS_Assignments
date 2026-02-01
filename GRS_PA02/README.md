# GRS_PA02 - Network I/O
## Name- Harsha Verma
## Roll No-MT25024

## Part A1
### Overview
This section will implement a TCP-based client-server program with the following features:
- The **server** will be implemented in a *thread-per-client* fashion using `pthread`
- The **client** will send messages of fixed size continuously for a specified period of time
- Both the client and server programs will run in **separate Linux network namespaces** to simulate a practical distributed environment without the need for virtualization
- Part A1 implements the TCP client/server skeleton: thread-per-client server + fixed-size message transfer + runtime parameterization + namespaces. The 8-field heap-allocated message structure is implemented in Part A2.

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
```

### Running the Server
Running the the server inside the server namespace:
```bash
sudo ip netns exec ns_s ./server <msg_size>
```
eg: 
```bash
sudo ip netns exec ns_s ./server 65536
```

### Running the Client 
Run the client inside the client namespace:
```bash
sudo ip netns exec ns_c ./client <server_ip> <port> <msg_size> <threads> <duration_sec>
```
eg:
```bash
sudo ip netns exec ns_c ./client 10.200.1.1 8989 65536 4 10
```
Each client thread reports throughput at the end of execution.

### Delete the namespaces
```bash
sudo ip netns del ns_s 2>/dev/null
sudo ip netns del ns_c 2>/dev/null
sudo ip link del veth_s 2>/dev/null
```

