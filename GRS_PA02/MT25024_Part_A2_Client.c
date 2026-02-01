/*
AI USAGE DECLARATION – MT25024_Part_A2_Client.c (PA02, Graduate Systems)

AI tools (ChatGPT) were used as a supportive aid for this component in the following ways:
- Clarifying recvmsg()/msghdr/iovec for scatter-gather receive
- Designing a duration-based loop and per-thread throughput reporting
- Reviewing safe runtime parameter parsing and EINTR handling

Representative prompts used include:
- "How to use recvmsg with iovec to receive into multiple buffers"
- "How to implement multi-threaded TCP client with pthreads"
- "How to measure throughput over a fixed duration in C"

All code in this file was written, reviewed, and fully understood.
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
#include <sys/uio.h>

typedef struct {
    char server_ip[64];
    int port;
    size_t msgSize;     // total bytes expected from server per iteration (8 fields sum to msgSize)
    int duration;       // seconds
} client_args_t;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

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

/* recvmsg() until all bytes across iovecs are received */
static int recvmsg_all(int fd, struct iovec *iov_in, int iovcnt_in) {
    struct iovec iov[8];
    if (iovcnt_in > 8) return -1;
    memcpy(iov, iov_in, (size_t)iovcnt_in * sizeof(struct iovec));
    int iovcnt = iovcnt_in;

    while (iovcnt > 0) {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = iov;
        msg.msg_iovlen = (size_t)iovcnt;

        ssize_t n = recvmsg(fd, &msg, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return 0; // closed

        size_t got = (size_t)n;

        // Consume 'got' bytes from iov[]
        size_t left = got;
        int idx = 0;
        while (idx < iovcnt && left > 0) {
            if (left >= iov[idx].iov_len) {
                left -= iov[idx].iov_len;
                idx++;
            } else {
                iov[idx].iov_base = (char*)iov[idx].iov_base + left;
                iov[idx].iov_len -= left;
                left = 0;
            }
        }
        if (idx > 0) {
            memmove(iov, iov + idx, (size_t)(iovcnt - idx) * sizeof(struct iovec));
            iovcnt -= idx;
        }
    }
    return 1;
}

static void *client_thread(void *arg) {
    client_args_t *cfg = (client_args_t*)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return NULL; }

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

    // Allocate 8 heap fields to receive into (matches server's 8 fields)
    size_t base = cfg->msgSize / 8;
    size_t rem  = cfg->msgSize % 8;

    char *field[8];
    size_t flen[8];
    for (int i = 0; i < 8; i++) {
        flen[i] = base + (i == 7 ? rem : 0);
        if (flen[i] == 0) flen[i] = 1;
        field[i] = (char*)malloc(flen[i]);
        if (!field[i]) {
            perror("malloc");
            for (int j = 0; j < i; j++) free(field[j]);
            close(sock);
            return NULL;
        }
    }

    struct iovec iov[8];
    for (int i = 0; i < 8; i++) {
        iov[i].iov_base = field[i];
        iov[i].iov_len  = flen[i];
    }

    // Client sends triggers continuously for 'duration', and receives the server message each iteration
    char trigger[8] = { 'P','I','N','G','P','I','N','G' };

    double start = now_sec();
    double end = start + cfg->duration;

    unsigned long long bytes_rx = 0;
    unsigned long long bytes_tx = 0;

    while (now_sec() < end) {
        if (send_all(sock, trigger, sizeof(trigger)) < 0) {
            perror("send");
            break;
        }
        bytes_tx += sizeof(trigger);

        int rc = recvmsg_all(sock, iov, 8);
        if (rc == 0) break;          // server closed
        if (rc < 0) { perror("recvmsg"); break; }

        bytes_rx += cfg->msgSize;
    }

    shutdown(sock, SHUT_WR);
    close(sock);

    for (int i = 0; i < 8; i++) free(field[i]);

    double elapsed = now_sec() - start;
    if (elapsed <= 0) elapsed = 1e-9;

    double gbps_rx = (bytes_rx * 8.0) / (elapsed * 1e9);
    fprintf(stderr,
            "[A2 client thread] rx_bytes=%llu tx_bytes=%llu time=%.2f sec rx_throughput=%.3f Gbps\n",
            bytes_rx, bytes_tx, elapsed, gbps_rx);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr,
            "Usage: %s <server_ip> <port> <msgSize> <threads> <duration_sec>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    size_t msgSize = (size_t)strtoul(argv[3], NULL, 10);
    int threads = atoi(argv[4]);
    int duration = atoi(argv[5]);

    if (threads <= 0) { fprintf(stderr, "threads must be > 0\n"); return 1; }
    if (duration <= 0) { fprintf(stderr, "duration must be > 0\n"); return 1; }
    if (msgSize < 8) { fprintf(stderr, "msgSize must be >= 8\n"); return 1; }

    client_args_t cfg;
    snprintf(cfg.server_ip, sizeof(cfg.server_ip), "%s", server_ip);
    cfg.port = port;
    cfg.msgSize = msgSize;
    cfg.duration = duration;

    pthread_t *tids = (pthread_t*)malloc(sizeof(pthread_t) * (size_t)threads);
    if (!tids) { perror("malloc"); return 1; }

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