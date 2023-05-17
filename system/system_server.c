#define _POSIX_C_SOURCE 200809L
#define BUFSIZE 1024
#define QUEUE_NUM 4
#define THREAD_NUM 5
#define DISK_USAGE "df -H ./"
#include <system_server.h>

mqd_t mqs[QUEUE_NUM];

static int toy_timer = 0;
static bool global_timer_stopped = false;
pthread_mutex_t toy_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t global_timer_sem;

static void sigalrm_handler(int sig, siginfo_t *si, void *uc)
{
  sem_post(&global_timer_sem);
}

static void system_timeout_handler()
{
  pthread_mutex_lock(&toy_timer_mutex);
  toy_timer++;
  printf("toy_timer: %d\n", toy_timer);
  pthread_mutex_unlock(&toy_timer_mutex);
}

static void* timer_thread(void* arg)
{
  int thread_id = (int)arg;
  signal(SIGALRM, sigalrm_handler); // 1. 알림 시그널 핸들러 수행
  set_periodic_timer(1, 1);

  printf("thread %d is running\n", thread_id);

  while (!global_timer_stopped) {
    int rc = sem_wait(&global_timer_sem);

    if (rc == -1 && errno == EINTR) continue;

		if (rc == -1) {
		    perror("sem_wait");
		    exit(-1);
		}

    system_timeout_handler(); // 2. 시스템 타임아웃 핸들러 수행
    break;
  }

  exit(EXIT_SUCCESS);
}

void* watchdog_thread(void* arg)
{
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

void* monitor_thread(void* arg)
{
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

void* disk_service_thread(void* arg)
{
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

    // 디스크 사용량 출력
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

void* camera_service_thread(void* arg)
{
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

    // 카메라로 사진 찍기
    if (strcmp(msg.msg_type, "CAMERA_TAKE_PICTURE") == 0) {
      toy_camera_take_picture();
    }
  }

  exit(EXIT_SUCCESS);
}

int posix_sleep_ms(unsigned int timeout_ms)
{
  struct timespec sleep_time;

  sleep_time.tv_sec = timeout_ms / MILLISEC_PER_SECOND;
  sleep_time.tv_nsec = (timeout_ms % MILLISEC_PER_SECOND) * (NANOSEC_PER_USEC * USEC_PER_MILLISEC);

  return nanosleep(&sleep_time, NULL);
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

  pthread_t threads[THREAD_NUM];
  void* (*thread_funcs[THREAD_NUM])(void*) = {watchdog_thread, monitor_thread, disk_service_thread, camera_service_thread, timer_thread};
  char* mq_names[QUEUE_NUM] = {"/watchdog_queue", "/monitor_queue", "/disk_queue", "/camera_queue"};

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = sigalrm_handler;
  sigemptyset(&sa.sa_mask);

  // SIGALRM 핸들러 등록
  if (sigaction(SIGALRM, &sa, NULL) == -1) {
    perror("sigaction error");
    exit(-1);
  }

  // 5초마다 SIGALRM 시그널 발생
  set_periodic_timer(5, 0);

  for (int i = 0; i < THREAD_NUM; i++) {
    if (pthread_create(&threads[i], NULL, thread_funcs[i], i) != 0) {
      perror("pthread_create error");
      exit(-1);
    }
  }

  // 메시지 큐 열기
  for (int i = 0; i < QUEUE_NUM; i++) {
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
