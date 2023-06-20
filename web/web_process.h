#ifndef _WEB_PROCESS_H
#define _WEB_PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t create_web_process();
void init_web_process();

#endif