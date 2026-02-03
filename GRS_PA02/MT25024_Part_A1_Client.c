/*
AI USAGE DECLARATION – MT25024_Part_A1_Client.c (PA02, Graduate Systems)

AI tools (ChatGPT) were used as a supportive aid for this component in the following ways:
- Understanding client-side TCP socket connection setup in C
- Clarifying continuous send loops and partial send handling
- Designing multi-threaded client behavior using pthreads
- Understanding duration-based benchmarking for throughput measurement
- Debugging socket send logic and runtime parameter handling
- Adding RTT/latency measurement using clock_gettime(CLOCK_MONOTONIC)

Representative prompts used include:
- "How to implement a multi-threaded TCP client in C"
- "How to send fixed-size messages continuously over TCP"
- "How to handle partial send() in C sockets"
- "How to parameterize message size and duration at runtime"
- "How to measure RTT in microseconds with clock_gettime"

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
#include <sys/time.h>      // timeval
#include <time.h>
#include <unistd.h>

typedef struct {
    char server_ip[64];
    int port;
    size_t msgSize;
    int duration;
} client_args_t;

/* monotonic clock in seconds */
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/*
 * send_all with bounded blocking (SO_SNDTIMEO).
 * Returns:
 *   0  success
 *  -2  timed out (caller may retry until deadline)
 *  -1  fatal error
 */
static int send_all(int fd, const void *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, (const char*)buf + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -2;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int recv_all_until(int fd, void *buf, size_t len, double deadline_sec) {
    size_t got = 0;

    while (got < len) {
        // ALWAYS enforce deadline
        if (now_sec() >= deadline_sec)
            return -2;

        ssize_t r = recv(fd, (char*)buf + got, len - got, 0);
        if (r == 0)
            return 0;   // peer closed

        if (r < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;   // bounded by deadline check above

            return -1;      // fatal error
        }

        got += (size_t)r;
    }

    return 1;   // full message received
}

static void *client_thread(void *arg) {
    client_args_t *cfg = (client_args_t*)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return NULL; }

    // Optional: reduce latency for small messages
    int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    // Bounded blocking so perf/slowdowns can't hang forever
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

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

    char *msgBuf = (char*)malloc(cfg->msgSize);
    if (!msgBuf) {
        perror("malloc msgBuf");
        close(sock);
        return NULL;
    }

    char trigger[8] = {'P','I','N','G','P','I','N','G'};

    double start = now_sec();
    double end = start + cfg->d/*
AI USAGE DECLARATION – MT25024_Part_A1_Client.c (PA02, Graduate Systems)

AI tools (ChatGPT) were used as a supportive aid for this component in the following ways:
- Understanding client-side TCP socket connection setup in C
- Clarifying continuous send loops and partial send handling
- Designing multi-threaded client behavior using pthreads
- Understanding duration-based benchmarking for throughput measurement
- Debugging socket send logic and runtime parameter handling
- Adding RTT/latency measurement using clock_gettime(CLOCK_MONOTONIC)

Representative prompts used include:
- "How to implement a multi-threaded TCP client in C"
- "How to send fixed-size messages continuously over TCP"
- "How to handle partial send() in C sockets"
- "How to parameterize message size and duration at runtime"
- "How to measure RTT in microseconds with clock_gettime"

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
#include <sys/time.h>      // timeval
#include <time.h>
#include <unistd.h>

typedef struct {
    char server_ip[64];
    int port;
    size_t msgSize;
    int duration;
} client_args_t;

/* monotonic clock in seconds */
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/*
 * send_all with bounded blocking (SO_SNDTIMEO).
 * Returns:
 *   0  success
 *  -2  timed out (caller may retry until deadline)
 *  -1  fatal error
 */
static int send_all(int fd, const void *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, (const char*)buf + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -2;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int recv_all_until(int fd, void *buf, size_t len, double deadline_sec) {
    size_t got = 0;

    while (got < len) {
        // ALWAYS enforce deadline
        if (now_sec() >= deadline_sec)
            return -2;

        ssize_t r = recv(fd, (char*)buf + got, len - got, 0);
        if (r == 0)
            return 0;   // peer closed

        if (r < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;   // bounded by deadline check above

            return -1;      // fatal error
        }

        got += (size_t)r;
    }

    return 1;   // full message received
}

static void *client_thread(void *arg) {
    client_args_t *cfg = (client_args_t*)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return NULL; }

    // reduce latency for small messages
    int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    // Bounded blocking so perf/slowdowns can't hang forever
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

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

    unsigned long long msg_count = 0;
    double total_rtt_us = 0.0;
    double max_rtt_us = 0.0;

    while (now_sec() < end) {
        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);

        int sret = send_all(sock, trigger, sizeof(trigger));
        if (sret == -2) continue;     // timed out, retry until benchmark ends
        if (sret < 0) { perror("send"); break; }
        bytes_tx += sizeof(trigger);

        int rc = recv_all_until(sock, msgBuf, cfg->msgSize, end);
        if (rc == -2) continue;           // retry until duration expires
        if (rc == 0) break;           // server closed
        if (rc < 0) { perror("recv"); break; }

        clock_gettime(CLOCK_MONOTONIC, &t2);

        double rtt_us =
            (t2.tv_sec - t1.tv_sec) * 1e6 +
            (t2.tv_nsec - t1.tv_nsec) / 1e3;

        total_rtt_us += rtt_us;
        msg_count++;
        if (rtt_us > max_rtt_us) max_rtt_us = rtt_us;

        bytes_rx += cfg->msgSize;
    }

    shutdown(sock, SHUT_WR);
    close(sock);
    free(msgBuf);

    double elapsed = now_sec() - start;
    if (elapsed <= 0) elapsed = 1e-9;

    double gbps_rx = (bytes_rx * 8.0) / (elapsed * 1e9);
    double avg_rtt_us = (msg_count > 0) ? (total_rtt_us / (double)msg_count) : 0.0;

    fprintf(stderr,
        "[A1 client thread] rx_bytes=%llu tx_bytes=%llu msgs=%llu time=%.2f sec "
        "rx_throughput=%.3f Gbps avg_rtt=%.2f us max_rtt=%.2f us\n",
        bytes_rx, bytes_tx, msg_count, elapsed, gbps_rx, avg_rtt_us, max_rtt_us);

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
}uration;

    unsigned long long bytes_tx = 0;
    unsigned long long bytes_rx = 0;

    unsigned long long msg_count = 0;
    double total_rtt_us = 0.0;
    double max_rtt_us = 0.0;

    while (now_sec() < end) {
        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);

        int sret = send_all(sock, trigger, sizeof(trigger));
        if (sret == -2) continue;     // timed out, retry until benchmark ends
        if (sret < 0) { perror("send"); break; }
        bytes_tx += sizeof(trigger);

        int rc = recv_all_until(sock, msgBuf, cfg->msgSize, end);
        if (rc == -2) continue;           // retry until duration expires
        if (rc == 0) break;           // server closed
        if (rc < 0) { perror("recv"); break; }

        clock_gettime(CLOCK_MONOTONIC, &t2);

        double rtt_us =
            (t2.tv_sec - t1.tv_sec) * 1e6 +
            (t2.tv_nsec - t1.tv_nsec) / 1e3;

        total_rtt_us += rtt_us;
        msg_count++;
        if (rtt_us > max_rtt_us) max_rtt_us = rtt_us;

        bytes_rx += cfg->msgSize;
    }

    shutdown(sock, SHUT_WR);
    close(sock);
    free(msgBuf);

    double elapsed = now_sec() - start;
    if (elapsed <= 0) elapsed = 1e-9;

    double gbps_rx = (bytes_rx * 8.0) / (elapsed * 1e9);
    double avg_rtt_us = (msg_count > 0) ? (total_rtt_us / (double)msg_count) : 0.0;

    fprintf(stderr,
        "[A1 client thread] rx_bytes=%llu tx_bytes=%llu msgs=%llu time=%.2f sec "
        "rx_throughput=%.3f Gbps avg_rtt=%.2f us max_rtt=%.2f us\n",
        bytes_rx, bytes_tx, msg_count, elapsed, gbps_rx, avg_rtt_us, max_rtt_us);

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
