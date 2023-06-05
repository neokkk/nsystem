TARGET = toy_system

SYSTEM = ./system
UI = ./ui
WEB_SERVER = ./web_server
HAL = ./hal

INCLUDES = -I./ -I$(SYSTEM) -I$(UI) -I$(WEB_SERVER) -I$(HAL)

CC = gcc
CXX = g++
CXXLIBS = -lpthread -lm -lrt -lseccomp
CXXFLAGS = $(INCLUDEDIRS) -g -O0 -std=c++20

objects = main.o system_server.o web_server.o input.o gui.o sem.o timer.o shared_memory.o dump_state.o hardware.o
cxx_objects = 
shared_libs = libcamera.oem.so libcamera.toy.so

$(TARGET): $(objects) $(cxx_objects) $(shared_libs)
	$(CXX) -o $(TARGET) $(objects) $(cxx_objects) $(CXXLIBS)

main.o: main.c
	$(CC) -g $(INCLUDES) -c main.c

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

dump_state.o: $(SYSTEM)/dump_state.h $(SYSTEM)/dump_state.c
	$(CC) -g $(INCLUDES) -c ./system/dump_state.c

hardware.o: $(HAL)/hardware.c
	$(CC) -g $(INCLUDES) -c  $(HAL)/hardware.c

.PHONY: libcamera.oem.so
libcamera.oem.so:
	$(CC) -g -shared -fPIC -o libcamera.oem.so $(INCLUDES) $(CXXFLAGS) $(HAL)/oem/camera_HAL_oem.cpp $(HAL)/oem/ControlThread.cpp

.PHONY: libcamera.toy.so
libcamera.toy.so:
	$(CC) -g -shared -fPIC -o libcamera.toy.so $(INCLUDES) $(CXXFLAGS) $(HAL)/toy/camera_HAL_toy.cpp $(HAL)/toy/ControlThread.cpp

.PHONY: clean
clean:
	rm -rf *.o *.gch
	rm -rf $(TARGET)
