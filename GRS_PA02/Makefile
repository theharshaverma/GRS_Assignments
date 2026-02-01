CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -pthread
LDFLAGS := -pthread

SERVER_SRC := MT25024_Part_A1_Server.c
CLIENT_SRC := MT25024_Part_A1_Client.c

SERVER_BIN := server
CLIENT_BIN := client

.PHONY: all clean

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) *.o