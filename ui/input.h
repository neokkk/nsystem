#ifndef _INPUT_H
#define _INPUT_H

#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#include <toy_message.h>

pid_t create_input();
int input();

void* sensor_thread(void* arg);
void* command_thread(void* arg);
void toy_loop();
char** toy_split_line(char* line);
char* toy_read_line(void);
int toy_execute(char** args);
int toy_shell(char** args);
int toy_exit(char** args);
int toy_message_queue(char** args);
int toy_mutex(char** args);
int toy_send(char** args);
int toy_num_builtins();
void sigsegv_handler(int sig_num, siginfo_t* info, void* ucontext);

#endif /* _INPUT_H */
