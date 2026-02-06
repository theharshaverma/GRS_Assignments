# Roll No- MT25024
# Makefile for PA02 – Graduate Systems (CSE638)

CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -pthread
LDFLAGS :=

BINS := a1_server a1_client a2_server a2_client a3_server a3_client

.PHONY: all a1 a2 a3 clean

# -------------------------
# Default target
# -------------------------
all: a1 a2 a3

a1: a1_server a1_client
a2: a2_server a2_client
a3: a3_server a3_client

# -------------------------
# Build rules
# -------------------------
a1_server: MT25024_Part_A1_Server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

a1_client: MT25024_Part_A1_Client.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

a2_server: MT25024_Part_A2_Server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

a2_client: MT25024_Part_A2_Client.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

a3_server: MT25024_Part_A3_Server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

a3_client: MT25024_Part_A3_Client.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# -------------------------
# Cleanup
# -------------------------
clean:
	rm -f $(BINS) *.o
	rm -rf results
