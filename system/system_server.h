#ifndef _SYSTEM_SERVER_H
#define _SYSTEM_SERVER_H

#include <assert.h>
#include <errno.h>
#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include <toy_message.h>
#include <timer.h>


pid_t create_system_server();
int system_server();

static void sigalrm_handler(int sig, siginfo_t* si, void* uc);
static void system_timeout_handler();
static void* timer_thread(void* not_used);
void* watchdog_thread(void* arg);
void* monitor_thread(void* arg);
void* disk_service_thread(void* arg);
void* camera_service_thread(void* arg);
void set_periodic_timer(long sec_delay, long usec_delay);

#endif /* _SYSTEM_SERVER_H */
