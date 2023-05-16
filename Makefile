CC = gcc
CFLAGS = -Wall -O -g -Isystem -Iui -Iweb_server

SUBDIRS = system ui web_server
BIN = toy_system

all: $(SUBDIRS)
	$(CC) -o $(BIN) $(wildcard *.c) $(wildcard */*.c) $(wildcard */*.h) $(CFLAGS)

clean:
	rm -f $(BIN)