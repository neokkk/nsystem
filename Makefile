CC = gcc
CXX = g++
CXXFLAGS = -g -O0 -std=c++20
CXXLIBS = -lpthread -lm -lrt

INCLUDES = -I./ -I./common -I./input -I./system -I./web

OBJS = main.o system_process.o web_process.o input_process.o common_timer.o common_mq.o
TARGET = toyy

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(CXXLIBS)

main.o: main.c
	$(CC) -g -c $< $(INCLUDES)

system_process.o: ./system/system_process.c ./system/system_process.h
	$(CC) -g -c $< $(INCLUDES)

web_process.o: ./web/web_process.c ./web/web_process.h
	$(CC) -g -c $< $(INCLUDES)

input_process.o: ./input/input_process.c ./input/input_process.h
	$(CC) -g -c $< $(INCLUDES)

common_timer.o: ./common/common_timer.c ./common/common_timer.h
	$(CC) -g -c $< $(INCLUDES)

common_mq.o: ./common/common_mq.c ./common/common_mq.h
	$(CC) -g -c $< $(INCLUDES)

# %.o: %.c
# 	$(CC) -g -c $< -o $@ $(INCLUDES)

.PHONY: clean
clean:
	rm -rf $(TARGET) *.o **/*.o
