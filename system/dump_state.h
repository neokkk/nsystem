#ifndef _DUMP_STATE_H
#define _DUMP_STATE_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/klog.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>

#define KLOG_CLOSE         0
#define KLOG_OPEN          1
#define KLOG_READ          2
#define KLOG_READ_ALL      3
#define KLOG_READ_CLEAR    4
#define KLOG_CLEAR         5
#define KLOG_CONSOLE_OFF   6
#define KLOG_CONSOLE_ON    7
#define KLOG_CONSOLE_LEVEL 8
#define KLOG_SIZE_UNREAD   9
#define KLOG_SIZE_BUFFER   10

#define KLOG_DISABLE 6
#define KLOG_ENABLE 7
#define KLOG_SETLEVEL 8
#define KLOG_UNREADSIZE 9
#define KLOG_WRITE 10

void dump_state();

#endif /* _DUMP_STATE_H */
