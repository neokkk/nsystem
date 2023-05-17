#ifndef _SYSTEM_SERVER_H
#define _SYSTEM_SERVER_H

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

int create_system_server();

#endif /* _SYSTEM_SERVER_H */
