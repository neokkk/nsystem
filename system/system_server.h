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
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <toy_message.h>

#define USEC_PER_SECOND         1000000
#define USEC_PER_MILLISEC       1000
#define NANOSEC_PER_SECOND      1000000000
#define NANOSEC_PER_USEC        1000
#define NANOSEC_PER_MILLISEC    1000000
#define MILLISEC_PER_TICK       10
#define MILLISEC_PER_SECOND     1000

pid_t create_system_server();
int system_server();

static void sigalrm_handler(int sig, siginfo_t *si, void *uc);
static void system_timeout_handler();
static void* timer_thread(void* not_used);
void* watchdog_thread(void* arg);
void* monitor_thread(void* arg);
void* disk_service_thread(void* arg);
void* camera_service_thread(void* arg);
int posix_sleep_ms(unsigned int timeout_ms);
void set_periodic_timer(long sec_delay, long usec_delay);

#endif /* _SYSTEM_SERVER_H */
