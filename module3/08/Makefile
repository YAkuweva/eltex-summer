CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = 

TARGET = udp_capture
SOURCES = main.c raw_udp_capture.c
HEADERS = raw_udp_capture.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET)

.PHONY: all clean run
