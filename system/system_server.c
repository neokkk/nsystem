#define _POSIX_C_SOURCE 200809L
#define BUFSIZE 1024
#define DISK_USAGE "df -H ./"
#include <system_server.h>

static int toy_timer = 0;
pthread_mutex_t system_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t system_loop_cond = PTHREAD_COND_INITIALIZER;
bool system_loop_exit = true;
mqd_t mqs[THREAD_NUM];

static void sigalrm_handler(int sig, siginfo_t *si, void *uc)
{
  toy_timer++;
  signal_exit();
}

int posix_sleep_ms(unsigned int timeout_ms)
{
  struct timespec sleep_time;

  sleep_time.tv_sec = timeout_ms / MILLISEC_PER_SECOND;
  sleep_time.tv_nsec = (timeout_ms % MILLISEC_PER_SECOND) * (NANOSEC_PER_USEC * USEC_PER_MILLISEC);

  printf("sleep\n");

  return nanosleep(&sleep_time, NULL);
}

void* watchdog_thread(void* arg) {
  int thread_id = (int)arg;
  toy_msg_t msg;
  ssize_t num_read;
  int priority;

  printf("thread %d is running\n", thread_id);

  while (1) {
    num_read = mq_receive(mqs[thread_id], (void*)&msg, sizeof(toy_msg_t), &priority);
    assert(num_read > 0);
    printf("watchdog_thread: 메시지가 도착했습니다.\n");
    printf("read %ld bytes; priority = %u\n", (long) num_read, priority);
    printf("msg.type: %d\n", msg.msg_type);
    printf("msg.param1: %d\n", msg.param1);
    printf("msg.param2: %d\n", msg.param2);
  }

  exit(EXIT_SUCCESS);
}

void* monitor_thread(void* arg) {
  int thread_id = (int)arg;
  toy_msg_t msg;
  ssize_t num_read;
  int priority;

  printf("thread %d is running\n", thread_id);

  while (1) {
    num_read = mq_receive(mqs[thread_id], (void*)&msg, sizeof(toy_msg_t), &priority);
    assert(num_read > 0); 
    printf("watchdog_thread: 메시지가 도착했습니다.\n");
    printf("read %ld bytes; priority = %u\n", (long) num_read, priority);
    printf("msg.type: %d\n", msg.msg_type);
    printf("msg.param1: %d\n", msg.param1);
    printf("msg.param2: %d\n", msg.param2);
  }

  exit(EXIT_SUCCESS);
}

void* disk_service_thread(void* arg) {
  int thread_id = (int)arg;
  FILE* fd;
  char buf[BUFSIZE];
  toy_msg_t msg;
  ssize_t num_read;
  int priority;

  printf("thread %d is running\n", thread_id);

  while (1) {
    num_read = mq_receive(mqs[thread_id], (void*)&msg, sizeof(toy_msg_t), &priority);
    assert(num_read > 0);
    printf("watchdog_thread: 메시지가 도착했습니다.\n");
    printf("read %ld bytes; priority = %u\n", (long) num_read, priority);
    printf("msg.type: %d\n", msg.msg_type);
    printf("msg.param1: %d\n", msg.param1);
    printf("msg.param2: %d\n", msg.param2);

    if ((fd = popen(DISK_USAGE, "r")) == NULL) {
      perror("popen error");
      exit(-1);
    }

    while (fgets(buf, BUFSIZE, fd) != NULL) {
      printf("%s", buf);
    }

    if (pclose(fd) < 0) {
      perror("pclose error");
      exit(-1);
    }
    posix_sleep_ms(10000);
  }

  exit(EXIT_SUCCESS);
}

void* camera_service_thread(void* arg) {
  int thread_id = (int)arg;
  toy_msg_t msg;;
  ssize_t num_read;
  int priority;

  printf("thread %d is running\n", thread_id);

   while (1) {
    num_read = mq_receive(mqs[thread_id], (void*)&msg, sizeof(toy_msg_t), &priority);
    assert(num_read > 0);
    printf("watchdog_thread: 메시지가 도착했습니다.\n");
    printf("read %ld bytes; priority = %u\n", (long) num_read, priority);
    printf("msg.type: %d\n", msg.msg_type);
    printf("msg.param1: %d\n", msg.param1);
    printf("msg.param2: %d\n", msg.param2);

    if (strcmp(msg.msg_type, "CAMERA_TAKE_PICTURE") == 0) {
      toy_camera_take_picture();
    }
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

void signal_exit()
{
  pthread_mutex_lock(&system_loop_mutex);
  system_loop_exit = true;
  pthread_cond_signal(&system_loop_cond);
  pthread_mutex_unlock(&system_loop_mutex);
}

int system_server()
{
  printf("system_server process is running\n");

  char* WATCHDOG_QUEUE = "/watchdog_queue";
  char* MONITOR_QUEUE = "/monitor_queue";
  char* DISK_QUEUE = "/disk_queue";
  char* CAMERA_QUEUE = "/camera_queue";

  struct sigaction sa;
  pthread_t threads[THREAD_NUM]; // watchdog, monitor, disk service, camera service
  void* (*thread_funcs[THREAD_NUM])(void*) = {watchdog_thread, monitor_thread, disk_service_thread, camera_service_thread};
  char* mq_names[THREAD_NUM] = {WATCHDOG_QUEUE, MONITOR_QUEUE, DISK_QUEUE, CAMERA_QUEUE};

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

  pthread_mutex_lock(&system_loop_mutex);
  while (!system_loop_exit) {
    pthread_cond_wait(&system_loop_cond, &system_loop_mutex);
  }
  pthread_mutex_unlock(&system_loop_mutex);

   for (int i = 0; i < THREAD_NUM; i++) {
    if ((mqs[i] = mq_open(mq_names[i], O_WRONLY)) < 0) {
      perror("mq_open error");
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

  printf("시스템 서버를 생성합니다.\n");

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
