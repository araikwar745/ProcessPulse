CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all clean
all: processpulse

processpulse: processpulse.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f processpulse
