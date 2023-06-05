#ifndef _SYSTEM_SERVER_H
#define _SYSTEM_SERVER_H

#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <dump_state.h>
#include <hardware.h>
#include <sensor_data.h>
#include <shared_memory.h>
#include <timer.h>
#include <toy_message.h>

pid_t create_system_server();
int system_server();

static void timer_expire_signal_handler();
static void system_timeout_handler();
static long get_directory_size(char *dirname);
void *timer_thread(void *not_used);
void *watchdog_thread(void *arg);
void *monitor_thread(void *arg);
void *disk_service_thread(void *arg);
void *camera_service_thread(void *arg);

#endif /* _SYSTEM_SERVER_H */
