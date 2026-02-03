/*
AI USAGE DECLARATION – MT25024_Part_A1_Server.c (PA02, Graduate Systems)

AI tools (ChatGPT) were used as a supportive aid for this component in the following ways:
- Clarifying the design of a thread-per-client TCP server using pthreads
- Understanding partial recv() behavior in TCP stream sockets
- Verifying correct handling of fixed-size message reception loops
- Understanding safe runtime parameterization and validation of message size
- Debugging error-handling logic (EINTR handling, client disconnect cases)

Representative prompts used include:
- "How to implement a thread-per-client TCP server in C using pthreads"
- "How does partial recv() work in TCP sockets?"
- "How to safely receive fixed-size messages over TCP in C"
- "Where should runtime validation of message size be performed in a server?"

All code in this file was written, reviewed, and fully understood.
The overall server design, implementation choices, and correctness reasoning
were performed independently, with AI used only for conceptual clarification
and debugging support.
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
#include <sys/time.h>      // timeval
#include <unistd.h>

#define SERVERPORT 8989
#define BUFSIZE 4096
#define SERVER_BACKLOG 128

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

static size_t g_msgSize = BUFSIZE;   // runtime message size (bytes)

/* recv exactly len bytes into buf (handles partial recv) */
static int recv_all(int fd, void *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t r = recv(fd, (char*)buf + got, len - got, 0);
        if (r == 0) return 0; // closed
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        got += (size_t)r;
    }
    return 1;
}

/*
 * send entire buffer (handles partial send)
 * Uses MSG_NOSIGNAL so server is not killed by SIGPIPE.
 * Also handles SO_SNDTIMEO timeout as a graceful failure.
 */
static int send_all(int fd, const void *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, (const char*)buf + sent, len - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; // timed out
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

/* 8-field heap-allocated message */
typedef struct {
    char *field[8];
    size_t flen[8];
} msg8_t;

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
        m->flen[i] = base + ((i == 7) ? rem : 0);
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
        memset(m->field[i], 'A' + i, m->flen[i]);
        if (m->flen[i] > 0) m->field[i][m->flen[i] - 1] = '\0'; // string-like
    }
}

static void *handle_connection(void *arg) {
    int clientSocket = *(int*)arg;
    free(arg);

    // Optional: reduce latency for small triggers
    int one = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    // Prevent a stuck send() if client stops reading
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Allocate 8 heap fields once per connection (requirement satisfied)
    msg8_t m;
    if (alloc_msg8(&m, g_msgSize) != 0) {
        perror("alloc_msg8");
        close(clientSocket);
        return NULL;
    }

    // Single contiguous buffer used for A1 send()
    char *msgBuf = (char*)malloc(g_msgSize);
    if (!msgBuf) {
        perror("malloc msgBuf");
        free_msg8(&m);
        close(clientSocket);
        return NULL;
    }

    fill_msg8(&m);

    char trigger[8];

    while (true) {
        int rc = recv_all(clientSocket, trigger, sizeof(trigger));
        if (rc == 0) break;                // client closed
        if (rc < 0) { perror("recv"); break; }

        // pack 8 heap fields -> one contiguous buffer EVERY trigger
        size_t off = 0;
        for (int i = 0; i < 8; i++) {
            memcpy(msgBuf + off, m.field[i], m.flen[i]);
            off += m.flen[i];
        }
        if (off != g_msgSize) {
            fprintf(stderr, "[A1 server] pack error: off=%zu msgSize=%zu\n", off, g_msgSize);
            break;
        }

        if (send_all(clientSocket, msgBuf, g_msgSize) < 0) {
            // client may have stopped reading / closed; exit this thread cleanly
            break;
        }
    }

    free(msgBuf);
    free_msg8(&m);
    close(clientSocket);
    return NULL;
}

int main(int argc, char **argv) {
    int serverSocket;
    SA_IN server_addr, client_addr;

    if (argc >= 2) {
        long v = strtol(argv[1], NULL, 10);
        if (v > 0) g_msgSize = (size_t)v;
    }

    if (g_msgSize < 8) {
        fprintf(stderr, "ERROR: Message size must be at least 8 bytes (got %zu)\n", g_msgSize);
        return 1;
    }

    const size_t MaxMsgSize = 10ULL * 1024ULL * 1024ULL; // 10 MB cap
    if (g_msgSize > MaxMsgSize) {
        fprintf(stderr, "ERROR: Message too big (max %zu bytes, got %zu)\n", MaxMsgSize, g_msgSize);
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("socket");
        return 1;
    }

    int one = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

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

    fprintf(stderr, "[A1 server] listening on port %d, msgSize=%zu bytes\n", SERVERPORT, g_msgSize);

    while (true) {
        socklen_t addr_size = sizeof(SA_IN);
        int clientSocket = accept(serverSocket, (SA*)&client_addr, &addr_size);
        if (clientSocket < 0) {
            perror("accept");
            continue;
        }

        pthread_t tid;
        int *pfd = (int*)malloc(sizeof(int));
        if (!pfd) {
            perror("malloc");
            close(clientSocket);
            continue;
        }
        *pfd = clientSocket;

        if (pthread_create(&tid, NULL, handle_connection, pfd) != 0) {
            perror("pthread_create");
            close(clientSocket);
            free(pfd);
            continue;
        }
        pthread_detach(tid);
    }

    close(serverSocket);
    return 0;
}
