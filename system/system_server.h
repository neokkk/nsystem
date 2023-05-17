#ifndef _SYSTEM_SERVER_H
#define _SYSTEM_SERVER_H

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

pid_t create_system_server();
int system_server();

static void sigalrm_handler(int sig, siginfo_t *si, void *uc);
int posix_sleep_ms(unsigned int timeout_ms);
void* watchdog_thread(void* arg);
void* monitor_thread(void* arg);
void* disk_service_thread(void* arg);
void* camera_service_thread(void* arg);
void set_periodic_timer(long sec_delay, long usec_delay);
void signal_exit();

#endif /* _SYSTEM_SERVER_H */
