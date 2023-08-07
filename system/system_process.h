#ifndef _SYSTEM_PROCESS_H_
#define _SYSTEM_PROCESS_H_

pid_t create_system_process();
void init_system_process();

void *timer_thread(void *);
void *disk_thread(void *);
void *camera_thread(void *);
void *monitor_thread(void *);
void *watchdog_thread(void *);

void sigalrm_handler(int sig);
long get_directory_size(char *dirname);

#endif