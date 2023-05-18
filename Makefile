TARGET = toy_system

SYSTEM = ./system
UI = ./ui
WEB_SERVER = ./web_server
HAL = ./hal

INCLUDES = -I./ -I$(SYSTEM) -I$(UI) -I$(WEB_SERVER) -I$(HAL) -Isem -Itimer

CC = gcc
CXX = g++
CXXLIBS = -lpthread -lm -lrt
CXXFLAGS = $(INCLUDEDIRS) -g -O0 -std=c++20

objects = main.o system_server.o web_server.o input.o gui.o sem.o timer.o shared_memory.o
cxx_objects = camera_HAL.o ControlThread.o

$(TARGET): $(objects) $(cxx_objects)
	$(CXX) -o $(TARGET) $(objects) $(cxx_objects) $(CXXLIBS)

main.o:  main.c
	$(CC) -g $(INCLUDES) -c main.c main.h

system_server.o: $(SYSTEM)/system_server.h $(SYSTEM)/system_server.c
	$(CC) -g $(INCLUDES) -c ./system/system_server.c

gui.o: $(UI)/gui.h $(UI)/gui.c
	$(CC) -g $(INCLUDES) -c $(UI)/gui.c

input.o: $(UI)/input.h $(UI)/input.c
	$(CC) -g $(INCLUDES) -c $(UI)/input.c sensor_data.h

web_server.o: $(WEB_SERVER)/web_server.h $(WEB_SERVER)/web_server.c
	$(CC) -g $(INCLUDES) -c $(WEB_SERVER)/web_server.c

sem.o: sem.h sem.c
	$(CC) -g $(INCLUDES) -c sem.c semun.h

timer.o: timer.h timer.c
	$(CC) -g $(INCLUDES) -c timer.c

shared_memory.o: shared_memory.h shared_memory.c
	$(CC) -g $(INCLUDES) -c shared_memory.c

camera_HAL.o: $(HAL)/camera_HAL.cpp
	$(CXX) -g $(INCLUDES) $(CXXFLAGS) -c  $(HAL)/camera_HAL.cpp

ControlThread.o: $(HAL)/ControlThread.cpp
	$(CXX) -g $(INCLUDES) $(CXXFLAGS) -c  $(HAL)/ControlThread.cpp

.PHONY: clean

clean:
	rm -rf *.o
	rm -rf $(TARGET)
