#ifndef _SYSTEM_PROCESS_H
#define _SYSTEM_PROCESS_H

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t create_system_process();
void init_system_process();

void *timer_thread(void *);
void *disk_thread(void *);
void *camera_thread(void *);
void *monitor_thread(void *);
void *watchdog_thread(void *);

void sigalrm_handler(int sig);

#endif