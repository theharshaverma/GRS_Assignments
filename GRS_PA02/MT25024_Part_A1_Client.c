/*
AI USAGE DECLARATION – MT25024_Part_A1_Client.c (PA02, Graduate Systems)

AI tools (ChatGPT) were used as a supportive aid for this component in the following ways:
- Understanding client-side TCP socket connection setup in C
- Clarifying continuous send loops and partial send handling
- Designing multi-threaded client behavior using pthreads
- Understanding duration-based benchmarking for throughput measurement
- Debugging socket send logic and runtime parameter handling

Representative prompts used include:
- "How to implement a multi-threaded TCP client in C"
- "How to send fixed-size messages continuously over TCP"
- "How to handle partial send() in C sockets"
- "How to parameterize message size and duration at runtime"

All code in this file was written, reviewed, and fully understood.
The experimental logic, threading design, and benchmarking methodology
were implemented independently, with AI used only for conceptual clarification
and debugging support.
*/

#define _POSIX_C_SOURCE 199309L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>   // TCP_NODELAY (optional)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    char server_ip[64];
    int port;
    size_t msgSize;
    int duration;
} client_args_t;

/* send entire buffer */
static int send_all(int fd, const void *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, (const char*)buf + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

/* recv exactly len bytes */
static int recv_all(int fd, void *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t r = recv(fd, (char*)buf + got, len - got, 0);
        if (r == 0) return 0;      // closed
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        got += (size_t)r;
    }
    return 1;
}

/* monotonic clock in seconds */
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void *client_thread(void *arg) {
    client_args_t *cfg = (client_args_t*)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return NULL; }

    // Optional: reduce latency for small triggers
    int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cfg->port);
    if (inet_pton(AF_INET, cfg->server_ip, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed for %s\n", cfg->server_ip);
        close(sock);
        return NULL;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return NULL;
    }

    // Receive buffer for whole msgSize response
    char *msgBuf = (char*)malloc(cfg->msgSize);
    if (!msgBuf) {
        perror("malloc msgBuf");
        close(sock);
        return NULL;
    }

    char trigger[8] = {'P','I','N','G','P','I','N','G'};

    double start = now_sec();
    double end = start + cfg->duration;

    unsigned long long bytes_tx = 0;
    unsigned long long bytes_rx = 0;

    while (now_sec() < end) {
        if (send_all(sock, trigger, sizeof(trigger)) < 0) {
            perror("send");
            break;
        }
        bytes_tx += sizeof(trigger);

        int rc = recv_all(sock, msgBuf, cfg->msgSize);
        if (rc == 0) break;           // server closed
        if (rc < 0) { perror("recv"); break; }

        bytes_rx += cfg->msgSize;
    }

    shutdown(sock, SHUT_WR);
    close(sock);
    free(msgBuf);

    double elapsed = now_sec() - start;
    if (elapsed <= 0) elapsed = 1e-9;

    double gbps_rx = (bytes_rx * 8.0) / (elapsed * 1e9);
    fprintf(stderr,
        "[A1 client thread] rx_bytes=%llu tx_bytes=%llu time=%.2f sec rx_throughput=%.3f Gbps\n",
        bytes_rx, bytes_tx, elapsed, gbps_rx);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr,
            "Usage: %s <server_ip> <port> <msgSize> <threads> <duration_sec>\n",
            argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    size_t msgSize = (size_t)strtoul(argv[3], NULL, 10);
    int threads = atoi(argv[4]);
    int duration = atoi(argv[5]);

    if (threads <= 0) { fprintf(stderr, "threads must be > 0\n"); return 1; }
    if (duration <= 0) { fprintf(stderr, "duration must be > 0\n"); return 1; }
    if (msgSize < 8) { fprintf(stderr, "Message size must be >= 8 bytes\n"); return 1; }

    pthread_t *tids = (pthread_t*)malloc(sizeof(pthread_t) * (size_t)threads);
    if (!tids) { perror("malloc"); return 1; }

    client_args_t cfg;
    snprintf(cfg.server_ip, sizeof(cfg.server_ip), "%s", server_ip);
    cfg.port = port;
    cfg.msgSize = msgSize;
    cfg.duration = duration;

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&tids[i], NULL, client_thread, &cfg) != 0) {
            perror("pthread_create");
            free(tids);
            return 1;
        }
    }

    for (int i = 0; i < threads; i++) pthread_join(tids[i], NULL);

    free(tids);
    return 0;
}