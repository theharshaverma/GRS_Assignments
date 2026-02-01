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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>

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
        sent += (size_t)n;
    }
    return 0;
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
    if (sock < 0) {
        perror("socket");
        return NULL;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cfg->port);
    inet_pton(AF_INET, cfg->server_ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return NULL;
    }

    char *buffer = malloc(cfg->msgSize);
    if (!buffer) {
        perror("malloc");
        close(sock);
        return NULL;
    }
    memset(buffer, 'A', cfg->msgSize);

    double start = now_sec();
    double end = start + cfg->duration;
    unsigned long long bytes_sent = 0;

    while (now_sec() < end) {
        if (send_all(sock, buffer, cfg->msgSize) < 0) {
            perror("send");
            break;
        }
        bytes_sent += cfg->msgSize;
    }

    shutdown(sock, SHUT_WR);
    close(sock);
    free(buffer);

    double elapsed = now_sec() - start;
    double gbps = (bytes_sent * 8.0) / (elapsed * 1e9);

    fprintf(stderr,
        "[client thread] bytes=%llu time=%.2f sec throughput=%.3f Gbps\n",
        bytes_sent, elapsed, gbps);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr,
            "Usage: %s <server_ip> <port> <msgSize> <threads> <duration_sec>\n",
            argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    size_t msgSize = (size_t)strtoul(argv[3], NULL, 10);
    int threads = atoi(argv[4]);
    int duration = atoi(argv[5]);

    if (msgSize < 8) {
        fprintf(stderr, "Message size must be >= 8 bytes\n");
        exit(1);
    }

    pthread_t *tids = malloc(sizeof(pthread_t) * threads);
    client_args_t cfg;
    snprintf(cfg.server_ip, sizeof(cfg.server_ip), "%s", server_ip);
    cfg.port = port;
    cfg.msgSize = msgSize;
    cfg.duration = duration;

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&tids[i], NULL, client_thread, &cfg) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }

    free(tids);
    return 0;
}