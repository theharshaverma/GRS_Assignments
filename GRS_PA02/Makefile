#Roll No- MT25024 Makefile for PA02
CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -pthread
LDFLAGS := 

A1_SERVER_SRC := MT25024_Part_A1_Server.c
A1_CLIENT_SRC := MT25024_Part_A1_Client.c

A2_SERVER_SRC := MT25024_Part_A2_Server.c
A2_CLIENT_SRC := MT25024_Part_A2_Client.c

A3_SERVER_SRC := MT25024_Part_A3_Server.c
A3_CLIENT_SRC := MT25024_Part_A3_Client.c

A1_SERVER_BIN := a1_server
A1_CLIENT_BIN := a1_client

A2_SERVER_BIN := a2_server
A2_CLIENT_BIN := a2_client

A3_SERVER_BIN := a3_server
A3_CLIENT_BIN := a3_client

.PHONY: all a1 a2 a3 clean

all: a1 a2 a3

a1: $(A1_SERVER_BIN) $(A1_CLIENT_BIN)

a2: $(A2_SERVER_BIN) $(A2_CLIENT_BIN)

a3: $(A3_SERVER_BIN) $(A3_CLIENT_BIN)

$(A1_SERVER_BIN): $(A1_SERVER_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(A1_CLIENT_BIN): $(A1_CLIENT_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(A2_SERVER_BIN): $(A2_SERVER_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(A2_CLIENT_BIN): $(A2_CLIENT_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(A3_SERVER_BIN): $(A3_SERVER_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(A3_CLIENT_BIN): $(A3_CLIENT_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(A1_SERVER_BIN) $(A1_CLIENT_BIN) $(A2_SERVER_BIN) $(A2_CLIENT_BIN) $(A3_SERVER_BIN) $(A3_CLIENT_BIN) *.o