#ifndef _COMMON_TIMER_H
#define _COMMON_TIMER_H

int posix_sleep_ms(unsigned int timeout_ms);
int set_periodic_timer(long sec_delay, long usec_delay);

#endif