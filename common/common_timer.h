#ifndef _COMMON_TIMER_H
#define _COMMON_TIMER_H

#include <sys/time.h>

int set_periodic_timer(long sec_delay, long usec_delay);

#endif