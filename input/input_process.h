#ifndef _INPUT_PROCESS_H
#define _INPUT_PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t create_input_process();
void init_input_process();

#endif