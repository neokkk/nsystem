#include <sys/time.h>
#include <timer.h>

#include <common_timer.h>

int posix_sleep_ms(unsigned int timeout_ms)
{
  struct timespec sleep_time;

  sleep_time.tv_sec = timeout_ms / MILLISEC_PER_SECOND;
  sleep_time.tv_nsec = (timeout_ms % MILLISEC_PER_SECOND) * (NANOSEC_PER_USEC * USEC_PER_MILLISEC);

  return nanosleep(&sleep_time, NULL);
}

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