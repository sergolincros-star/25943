CC     = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g
TARGET = main

all: $(TARGET) part1

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

part1: part1.c
	$(CC) $(CFLAGS) -o part1 part1.c

clean:
	rm -f $(TARGET) part1 *.o core

.PHONY: all clean
