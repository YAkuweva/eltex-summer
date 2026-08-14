CC = gcc
CFLAGS = -Wall -Wextra -g
OBJS = chat_utils.o

all: server client

server: server.c $(OBJS)
	$(CC) $(CFLAGS) -o server server.c $(OBJS)

client: client.c $(OBJS)
	$(CC) $(CFLAGS) -o client client.c $(OBJS)

chat_utils.o: chat_utils.c chat.h
	$(CC) $(CFLAGS) -c chat_utils.c

clean:
	rm -f server client *.o

.PHONY: all clean
