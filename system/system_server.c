#define _POSIX_C_SOURCE 200809L
#define THREAD_NUM 4
#include <system_server.h>

static int toy_timer = 0;

static void sigalrm_handler(int sig, siginfo_t *si, void *uc)
{
  printf("toy_timer: %d\n", toy_timer);
  toy_timer++;
}

// void set_periodic_timer(long sec_delay, long usec_delay)
// {
//   timer_t timerid;
//   printf("set periodic timer\n");

//   struct sigevent sev = {
//     .sigev_notify = SIGEV_SIGNAL,
//     .sigev_signo = SIGALRM,
//     .sigev_value = {
//       .sival_ptr = &timerid,
//     },
//   };

//   if (timer_create(CLOCK_REALTIME, &sev, &timerid) < 0) {
//     perror("timer_create error");
//     exit(-1);
//   }

//   struct itimerspec ts = {
//     .it_value = {
//       .tv_sec = 5,
//       .tv_nsec = 0,
//     },
//     .it_interval = {
//       .tv_sec = 5,
//       .tv_nsec = 0,
//     },
//   };

//   if (timer_settime(timerid, 0, &ts, NULL) < 0) {
//     perror("timer_settime error");
//     exit(-1);
//   }

//   exit(EXIT_SUCCESS);
// }

void* watchdog_thread(void* arg) {
  int thread_id = (int)arg;
  printf("thread %d is running\n", thread_id);

  while (1) {
    sleep(1);
  }

  exit(EXIT_SUCCESS);
}

void* monitor_thread(void* arg) {
  int thread_id = (int)arg;
  printf("thread %d is running\n", thread_id);

  while (1) {
    sleep(1);
  }

  exit(EXIT_SUCCESS);
}

void* disk_service_thread(void* arg) {
  int thread_id = (int)arg;
  printf("thread %d is running\n", thread_id);

  while (1) {
    sleep(1);
  }

  exit(EXIT_SUCCESS);
}

void* camera_service_thread(void* arg) {
  int thread_id = (int)arg;
  printf("thread %d is running\n", thread_id);

  while (1) {
    sleep(1);
  }

  exit(EXIT_SUCCESS);
}

void set_periodic_timer(long sec_delay, long usec_delay)
{
	struct itimerval itimer_val = {
    .it_interval = { .tv_sec = sec_delay, .tv_usec = usec_delay },
    .it_value = { .tv_sec = sec_delay, .tv_usec = usec_delay }
  };

	setitimer(ITIMER_REAL, &itimer_val, (struct itimerval*)0);
}

int system_server()
{
  printf("system_server process is running\n");

  struct sigaction sa;
  pthread_t threads[THREAD_NUM]; // watchdog, monitor, disk service, camera service
  void* (*thread_funcs[THREAD_NUM])(void*) = {watchdog_thread, monitor_thread, disk_service_thread, camera_service_thread};

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = sigalrm_handler;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGALRM, &sa, NULL) == -1) {
    perror("sigaction error");
    exit(-1);
  }

  set_periodic_timer(5, 0);

  for (int i = 0; i < THREAD_NUM; i++) {
    if (pthread_create(&threads[i], NULL, thread_funcs[i], i) != 0) {
      perror("pthread_create error");
      exit(-1);
    }
  }

  while (1) {
    sleep(1);
  }

  exit(EXIT_SUCCESS);
}

pid_t create_system_server()
{
  pid_t system_pid;
  const char *process_name = "system_server";

  switch (system_pid = fork()) {
    case -1:
      perror("system_server fork error");
      exit(-1);
    case 0:
      if (prctl(PR_SET_NAME, process_name) < 0) {
        perror("prctl error");
        exit(-1);
      }
      system_server();
      break;
    default:
      printf("system_server pid: %d\n", system_pid);
  }

  return system_pid;
}
