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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>

#define SERVERPORT 8989
#define BUFSIZE 4096
#define SERVER_BACKLOG 128

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

static size_t g_msgSize = BUFSIZE;   // runtime message size (bytes)

static void *handle_connection(void *arg);

int main(int argc, char **argv) {
    int serverSocket;
    SA_IN server_addr, client_addr;

    // Allow msg size from CLI: ./server <msgSize>
    if (argc >= 2) {
        long v = strtol(argv[1], NULL, 10);
        if (v > 0) g_msgSize = (size_t)v;
    }

    // Validate runtime message size
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

    // Avoid "Address already in use" on quick restarts
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

    fprintf(stderr, "[server] listening on port %d, msgSize=%zu bytes\n", SERVERPORT, g_msgSize);

    while (true) {
        socklen_t addr_size = sizeof(SA_IN);
        int clientSocket = accept(serverSocket, (SA*)&client_addr, &addr_size);
        if (clientSocket < 0) {
            perror("accept");
            continue;
        }

        // One thread per client
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

    // Unreachable in this infinite server but kept for completeness
    close(serverSocket);
    return 0;
}

static void *handle_connection(void *arg) {
    int clientSocket = *(int*)arg;
    free(arg);

    char *buffer = (char*)malloc(g_msgSize);
    if (!buffer) {
        perror("malloc");
        close(clientSocket);
        return NULL;
    }

    // Receive fixed-size messages repeatedly (C2S client sends continuously)
    while (true) {
        size_t got = 0;
        while (got < g_msgSize) {
            ssize_t r = recv(clientSocket, buffer + got, g_msgSize - got, 0);
            if (r == 0) {  // client closed
                free(buffer);
                close(clientSocket);
                return NULL;
            }
            if (r < 0) {
                if (errno == EINTR) continue;
                perror("recv");
                free(buffer);
                close(clientSocket);
                return NULL;
            }
            got += (size_t)r;
        }
        // One full message received loop again
    }
}