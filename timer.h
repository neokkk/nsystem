#include <sys/time.h>
#include <time.h>

#define USEC_PER_SECOND         1000000
#define USEC_PER_MILLISEC       1000
#define NANOSEC_PER_SECOND      1000000000
#define NANOSEC_PER_USEC        1000
#define NANOSEC_PER_MILLISEC    1000000
#define MILLISEC_PER_TICK       10
#define MILLISEC_PER_SECOND     1000

int posix_sleep_ms(unsigned int timeout_ms);
void set_periodic_timer(long sec_delay, long usec_delay);