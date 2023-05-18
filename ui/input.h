#ifndef _INPUT_H
#define _INPUT_H

#include <assert.h>
#include <errno.h>
#include <execinfo.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#include <wait.h>

#include <sensor_data.h>
#include <shared_memory.h>
#include <toy_message.h>
#include <timer.h>

typedef struct sensor_data_t {
  float altitude;
  float temperature;
  float pressure;
};

pid_t create_input();
int input();

static void sigsegv_handler(int sig_num, siginfo_t* info, void* ucontext);
int toy_num_builtins();
int toy_send(char** args);
int toy_mutex(char** args);
int toy_message_queue(char** args);
int toy_exit(char** args);
int toy_shell(char** args);
int toy_execute(char** args);
char** toy_split_line(char* line);
char* toy_read_line(void);
void toy_loop();
void* sensor_thread(void* arg);
void* command_thread(void* arg);

#endif /* _INPUT_H */
