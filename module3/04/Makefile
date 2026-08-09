CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = producer_consumer
SOURCES = main.c ipc_utils.c producer.c consumer.c
HEADERS = ipc_utils.h
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

run3: $(TARGET)
	./$(TARGET) 3

run5: $(TARGET)
	./$(TARGET) 5

.PHONY: all clean run run3 run5
