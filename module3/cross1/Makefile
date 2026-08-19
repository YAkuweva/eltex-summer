CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGETS = server client

all: $(TARGETS)

server: server.c checksum.c common.h
	$(CC) $(CFLAGS) -o server server.c checksum.c

client: client.c checksum.c common.h
	$(CC) $(CFLAGS) -o client client.c checksum.c

clean:
	rm -f $(TARGETS)

run-server:
	sudo ./server

run-client1:
	sudo ./client 127.0.0.1 12345

run-client2:
	sudo ./client 127.0.0.1 12346

.PHONY: all clean run-server run-client1 run-client2
