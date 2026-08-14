CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = chat
SOURCES = chat.c network.c utils.c
HEADERS = chat.h network.h utils.h
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET) Alice

.PHONY: all clean run help
