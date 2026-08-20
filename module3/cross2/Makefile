CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGETS = taxi driver

all: $(TARGETS)

taxi: taxi.c common.h
	$(CC) $(CFLAGS) -o taxi taxi.c

driver: driver.c common.h
	$(CC) $(CFLAGS) -o driver driver.c

clean:
	rm -f $(TARGETS) /tmp/taxi_driver_*

run:
	./taxi

.PHONY: all clean run
