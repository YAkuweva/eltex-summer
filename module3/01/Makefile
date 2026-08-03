CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = 01
SOURCES = main.c process.c file_utils.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = process.h file_utils.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) *.copy

run: $(TARGET)
	./$(TARGET) file1.txt file2.txt

.PHONY: all clean run
