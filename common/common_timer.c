#include <common_timer.h>

int set_periodic_timer(long sec_delay, long usec_delay)
{
	struct itimerval itimer = {
		.it_interval = {
			.tv_sec = sec_delay,
			.tv_usec = usec_delay,
		},
		.it_value = {
			.tv_sec = sec_delay,
			.tv_usec = usec_delay,
		}
	};

	return setitimer(ITIMER_REAL, &itimer, (struct itimerval *)0);
}