CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fPIC
LDFLAGS = -ldl
TARGET = calculator
PLUGINS_DIR = plugins
LIB_NAMES = add subtract multiply divide power

$(shell mkdir -p $(PLUGINS_DIR))

all: $(TARGET) plugins

$(TARGET): main.o calculator.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o calculator.o $(LDFLAGS)

main.o: main.c calculator.h
	$(CC) $(CFLAGS) -c main.c -o main.o

calculator.o: calculator.c calculator.h
	$(CC) $(CFLAGS) -c calculator.c -o calculator.o

plugins: $(LIB_NAMES:%=lib%.so)

lib%.so: lib%.c
	$(CC) $(CFLAGS) -shared -o $(PLUGINS_DIR)/$@ $<

clean:
	rm -f $(TARGET) *.o $(PLUGINS_DIR)/*.so

run: $(TARGET)
	LD_LIBRARY_PATH=$(PLUGINS_DIR):$$LD_LIBRARY_PATH ./$(TARGET)

.PHONY: all clean run plugins