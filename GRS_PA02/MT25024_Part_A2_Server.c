/*
AI USAGE DECLARATION – MT25024_Part_A2_Server.c (PA02, Graduate Systems)

AI tools (ChatGPT) were used as a supportive aid for this component in the following ways:
- Clarifying sendmsg()/msghdr/iovec for scatter-gather send
- Verifying partial sendmsg behavior and safe retry logic (EINTR)
- Reviewing thread-per-client TCP server structure using pthreads
- Reviewing runtime message-size validation

Representative prompts used include:
- "How to use sendmsg with iovec to send multiple buffers"
- "How to handle partial sendmsg in TCP sockets"
- "Thread-per-client TCP server in C using pthreads"

All code in this file was written, reviewed, and fully understood.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>   // TCP_NODELAY (optional)
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#define SERVERPORT 8989
#define SERVER_BACKLOG 128

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

typedef struct {
    char *field[8];
    size_t flen[8];
} msg8_t;

static size_t g_msgSize = 65536;   // total bytes across 8 fields

static void free_msg8(msg8_t *m) {
    for (int i = 0; i < 8; i++) {
        free(m->field[i]);
        m->field[i] = NULL;
        m->flen[i] = 0;
    }
}

static int alloc_msg8(msg8_t *m, size_t total) {
    memset(m, 0, sizeof(*m));
    if (total < 8) return -1;

    size_t base = total / 8;
    size_t rem  = total % 8;

    for (int i = 0; i < 8; i++) {
        m->flen[i] = base + (i == 7 ? rem : 0);
        if (m->flen[i] == 0) m->flen[i] = 1;

        m->field[i] = (char*)malloc(m->flen[i]);
        if (!m->field[i]) {
            free_msg8(m);
            return -1;
        }
    }
    return 0;
}

static void fill_msg8(msg8_t *m) {
    for (int i = 0; i < 8; i++) {
        memset(m->field[i], (int)('A' + i), m->flen[i]);
    }
}

/* sendmsg() until all bytes across iovecs are sent */
static int sendmsg_all(int fd, const struct iovec *iov_in, int iovcnt_in) {
    if (iovcnt_in > 8) return -1;

    struct iovec iov[8];
    memcpy(iov, iov_in, (size_t)iovcnt_in * sizeof(struct iovec));
    int iovcnt = iovcnt_in;

    while (iovcnt > 0) {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = iov;
        msg.msg_iovlen = (size_t)iovcnt;

        ssize_t n = sendmsg(fd, &msg, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;

        size_t sent = (size_t)n;

        // Consume 'sent' bytes
        size_t left = sent;
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
    return 0;
}

/* recv exactly len bytes into buf */
static int recv_all(int fd, void *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t r = recv(fd, (char*)buf + got, len - got, 0);
        if (r == 0) return 0;
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        got += (size_t)r;
    }
    return 1;
}

static void *handle_connection(void *arg) {
    int clientSocket = *(int*)arg;
    free(arg);

    // Optional: reduce trigger latency
    int one = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    msg8_t m;
    if (alloc_msg8(&m, g_msgSize) != 0) {
        perror("alloc_msg8");
        close(clientSocket);
        return NULL;
    }

    // Build iov pointing to the 8 heap fields
    struct iovec iov[8];
    for (int i = 0; i < 8; i++) {
        iov[i].iov_base = m.field[i];
        iov[i].iov_len  = m.flen[i];
    }

    // fill once per connection (no 64KB memset per trigger)
    fill_msg8(&m);

    char trigger[8];

    while (true) {
        int rc = recv_all(clientSocket, trigger, sizeof(trigger));
        if (rc == 0) break;
        if (rc < 0) { perror("recv"); break; }

        if (sendmsg_all(clientSocket, iov, 8) != 0) {
            perror("sendmsg");
            break;
        }
    }

    free_msg8(&m);
    close(clientSocket);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        long v = strtol(argv[1], NULL, 10);
        if (v > 0) g_msgSize = (size_t)v;
    }

    if (g_msgSize < 8) {
        fprintf(stderr, "ERROR: msgSize must be >= 8 bytes (got %zu)\n", g_msgSize);
        return 1;
    }

    const size_t MaxMsgSize = 10ULL * 1024ULL * 1024ULL;
    if (g_msgSize > MaxMsgSize) {
        fprintf(stderr, "ERROR: msgSize too big (max %zu, got %zu)\n", MaxMsgSize, g_msgSize);
        return 1;
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) { perror("socket"); return 1; }

    int one = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    SA_IN server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVERPORT);

    if (bind(serverSocket, (SA*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, SERVER_BACKLOG) < 0) {
        perror("listen");
        close(serverSocket);
        return 1;
    }

    fprintf(stderr, "[A2 server] listening on %d, msgSize=%zu bytes (8 fields)\n",
            SERVERPORT, g_msgSize);

    while (true) {
        socklen_t addr_size = sizeof(SA_IN);
        int clientSocket = accept(serverSocket, (SA*)&client_addr, &addr_size);
        if (clientSocket < 0) { perror("accept"); continue; }

        pthread_t tid;
        int *pfd = (int*)malloc(sizeof(int));
        if (!pfd) { perror("malloc"); close(clientSocket); continue; }
        *pfd = clientSocket;

        if (pthread_create(&tid, NULL, handle_connection, pfd) != 0) {
            perror("pthread_create");
            close(clientSocket);
            free(pfd);
            continue;
        }
        pthread_detach(tid);
    }
}