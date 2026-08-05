CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = phonebook
TEST_TARGET = test_phonebook

all: $(TARGET)

$(TARGET): main.o contacts.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o contacts.o

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): test.o contacts.o
	$(CC) $(CFLAGS) -o $(TEST_TARGET) test.o contacts.o

main.o: main.c contacts.h
	$(CC) $(CFLAGS) -c main.c -o main.o

contacts.o: contacts.c contacts.h
	$(CC) $(CFLAGS) -c contacts.c -o contacts.o

test.o: test.c contacts.h
	$(CC) $(CFLAGS) -c test.c -o test.o

clean:
	rm -f $(TARGET) $(TEST_TARGET) *.o

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run test