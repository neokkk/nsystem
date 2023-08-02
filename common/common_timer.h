#ifndef _COMMON_TIMER_H
#define _COMMON_TIMER_H

#define USEC_PER_SECOND         1000000  /* one million */ 
#define USEC_PER_MILLISEC       1000     /* one thousand */
#define NANOSEC_PER_SECOND      1000000000 /* one BILLLION */
#define NANOSEC_PER_USEC        1000     /* one thousand */
#define NANOSEC_PER_MILLISEC    1000000  /* one million */
#define MILLISEC_PER_TICK       10
#define MILLISEC_PER_SECOND     1000

int posix_sleep_ms(unsigned int timeout_ms);
int set_periodic_timer(long sec_delay, long usec_delay);

#endif