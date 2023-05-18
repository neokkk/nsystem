#define _POSIX_C_SOURCE 200809L
#define BUFSIZE 1024
#define SYSTEM_THREAD_NUM 5
#define DISK_USAGE "df -H ./"
#define WATCH_DIR "./fs"
#define CAMERA_TAKE_PICTURE 1
#define SENSOR_DATA 1

#include <system_server.h>

static int toy_timer = 0;
static bool global_timer_stopped = false;
static pthread_mutex_t toy_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t global_timer_sem;
static sensor_data_t* sensor_data;
static mqd_t mqs[QUEUE_NUM];
static char* mq_names[QUEUE_NUM] = {"/watchdog_queue", "/monitor_queue", "/disk_queue", "/camera_queue"};

static void sigalrm_handler(int sig, siginfo_t *si, void *uc)
{
  sem_post(&global_timer_sem);
}

void system_timeout_handler()
{
  pthread_mutex_lock(&toy_timer_mutex);
  toy_timer++;
  printf("toy_timer: %d\n", toy_timer);
  pthread_mutex_unlock(&toy_timer_mutex);
}

void* timer_thread(void* arg)
{
  int thread_id = (int)arg;

  signal(SIGALRM, sigalrm_handler); // 1. 알림 시그널 핸들러 수행
  set_periodic_timer(1, 1);

  printf("timer thread(%d) is running\n", thread_id);

  while (!global_timer_stopped) {
    int rc = sem_wait(&global_timer_sem);

    if (rc == -1 && errno == EINTR) continue;

		if (rc == -1) {
      perror("global_timer sem_wait error\n");
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

  printf("watchdog thread(%d) is running\n", thread_id);

  while (1) {
    num_read = mq_receive(mqs[thread_id], (void*)&msg, sizeof(toy_msg_t), &priority);
    if (num_read < 0) continue;
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
  int shimid;

  printf("monitor thread(%d) is running\n", thread_id);

  while (1) {
    num_read = mq_receive(mqs[thread_id], (void*)&msg, sizeof(toy_msg_t), &priority);
    if (num_read < 0) continue;
    printf("monitor_thread: 메시지가 도착했습니다.\n");
    printf("read %ld bytes; priority = %u\n", (long) num_read, priority);
    printf("msg.type: %d\n", msg.msg_type);
    printf("msg.param1: %d\n", msg.param1);
    printf("msg.param2: %d\n", msg.param2);

    if (msg.msg_type == SENSOR_DATA) { // 센서 데이터를 받으면 공유 메모리에 저장된 정보를 불러온다.
      shimid = msg.param1;
      sensor_data = (sensor_data_t*)shm_attach(shimid);
      printf("sensor humidity: %d\n", sensor_data ->humidity);
      printf("sensor pressure: %d\n", sensor_data ->pressure);
      printf("sensor temperature: %d\n", sensor_data ->temperature);
      shm_detach(sensor_data);
    }
  }

  exit(EXIT_SUCCESS);
}

void* disk_service_thread(void* arg)
{
  int thread_id = (int)arg;
  char buf[BUFSIZE];
  char* p;
  ssize_t num_read;
  struct inotify_event* event;
  int watch_fd;
  int total_size = 0;

  printf("disk_service thread(%d) is running\n", thread_id);

  if ((watch_fd = inotify_init()) < 0) {
    perror("inotify_init error");
    exit(-1);
  }

  if (inotify_add_watch(watch_fd, WATCH_DIR, IN_CREATE) < 0) {
    perror("inotify_add_watch error");
    exit(-1);
  }

  while (1) {
    num_read = read(watch_fd, buf, BUFSIZE);
    printf("num_read: %d bytes\n", num_read);

    if (num_read == 0) {
      printf("read from inotify fd returned 0!\n");
      return 0;
    }

    if (num_read == -1) {
      perror("read error");
      exit(-1);
    }

    for (p = buf; p < buf + num_read; ) {
      event = (struct inotify_event*)p;
      if (event ->mask & IN_CREATE) {
        printf("The file %s was created.\n", event ->name);
        break;
      }
      p += sizeof(struct inotify_event) + event ->len;
    }
  }

  exit(EXIT_SUCCESS);
}

void* camera_service_thread(void* arg)
{
  int thread_id = (int)arg;
  toy_msg_t msg;;
  ssize_t num_read;
  int priority;

  printf("camera_service thread(%d) is running\n", thread_id);

   while (1) {
    num_read = mq_receive(mqs[thread_id], (void*)&msg, sizeof(toy_msg_t), &priority);
    if (num_read < 0) continue;
    printf("camera_service_thread: 메시지가 도착했습니다.\n");
    printf("read %ld bytes; priority = %u\n", (long) num_read, priority);
    printf("msg.type: %d\n", msg.msg_type);
    printf("msg.param1: %d\n", msg.param1);
    printf("msg.param2: %d\n", msg.param2);

    if (strcmp(msg.msg_type, CAMERA_TAKE_PICTURE) == 0) { // 카메라로 사진 찍기
      toy_camera_take_picture();
    }
  }

  exit(EXIT_SUCCESS);
}

int system_server()
{
  struct sigaction sa;
  pthread_t threads[SYSTEM_THREAD_NUM];
  void* (*thread_funcs[SYSTEM_THREAD_NUM])(void*) = {watchdog_thread, monitor_thread, disk_service_thread, camera_service_thread, timer_thread};

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = sigalrm_handler;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGALRM, &sa, NULL) == -1) { // SIGALRM 핸들러 등록
    perror("sigaction error\n");
    return -1;
  }

  set_periodic_timer(5, 0); // 5초마다 SIGALRM 시그널 발생

  for (int i = 0; i < QUEUE_NUM; i++) { // 메시지 큐 열기
    if ((mqs[i] = mq_open(mq_names[i], O_WRONLY)) < 0) {
      perror("mq_open error\n");
      return -1;
    }
  }

  printf("system thread num: %d\n", SYSTEM_THREAD_NUM);
  for (int i = 0; i < SYSTEM_THREAD_NUM; i++) {
    if (pthread_create(&threads[i], NULL, thread_funcs[i], i) != 0) {
      perror("pthread_create error\n");
      return -1;
    }
  }

  while (1) {
    sleep(1);
  }

  return 0;
}

pid_t create_system_server()
{
  pid_t system_pid;
  const char *process_name = "system_server";

  printf("시스템 서버를 생성합니다.\n");

  switch (system_pid = fork()) {
    case -1:
      perror("system_server fork error\n");
      exit(-1);
    case 0:
      if (prctl(PR_SET_NAME, process_name) < 0) {
        perror("prctl error\n");
        exit(-1);
      }
      system_server();
      break;
    default:
      printf("system_server pid: %d\n", system_pid);
  }

  return system_pid;
}
