CC = gcc
CXX = g++
CFLAGS = -Wall -O -g -Isystem -Iui -Iweb_server
CXXFLGAS = -O -g -std=c++20

CSUBDIRS = system ui web_server
CXXSUBDIRS = hal
BIN = toy_system

all: c cpp
	

c: $(SUBDIRS)
	$(CC) -o $@ $(wildcard *.c) $(wildcard */*.c) $(wildcard */*.h) $(CFLAGS)

cpp: $(CXXSUBDIRS)
	$(CXX) -o $@ $(BIN) $(wildcard */*.cpp) $(wildcard */*.h) $(CXXFLGAS)

clean:
	rm -f $(BIN)
	rm -f *.o