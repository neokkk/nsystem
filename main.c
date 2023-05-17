#define _POSIX_C_SOURCE 200809L
#define QUEUE_NUM 4
#define PROCESS_NUM 4
#include <main.h>

void sigchld_handler(int sig)
{
  int status, saved_errno;
  pid_t pid;

  saved_errno = errno;
  printf("caught SIGCHLD signal: %d\n", sig);

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    printf("child process %d terminated\n", pid);
  }

  if (pid == -1 && errno != ECHILD) {
    perror("waitpid error");
    exit(-1);
  }

  errno = saved_errno;
}

int create_message_queue(mqd_t* msgq_ptr, const char* queue_name, int num_messages, int message_size)
{
  mqd_t mq;
  struct mq_attr attr;
  int mq_flags = O_RDWR | O_CREAT | O_CLOEXEC;
  
  printf("%s name=%s nummsgs=%d\n", __func__, queue_name, num_messages);

  attr.mq_msgsize = message_size;
  attr.mq_maxmsg = num_messages;

  mq_unlink(queue_name);
  mq = mq_open(queue_name, mq_flags, 0666, &attr);

  if (mq < 0) {
    printf("%s queue=%s already exists so try to open\n", __func__, queue_name);
    mq = mq_open(queue_name, O_RDWR);
    assert(mq != (mqd_t)-1);
    printf("%s queue=%s opened successfully\n", __func__, queue_name);
    return -1;
  }

  return 0;
}

int main()
{
  pid_t pids[PROCESS_NUM];
  pid_t (*funcs[PROCESS_NUM])() = {create_system_server, create_web_server, create_input, create_gui};
  mqd_t mqs[QUEUE_NUM];
  char* mq_names[QUEUE_NUM] = {"/watchdog_queue", "/monitor_queue", "/disk_queue", "/camera_queue"};
  int status;
  struct sigaction sa;

  sa.sa_flags = 0;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction error");
    exit(-1);
  }

  printf("메인 함수입니다.\n");

  for (int i = 0; i < PROCESS_NUM; i++) {
    pids[i] = funcs[i]();
    printf("pid: %d\n", pids[i]);
  }

  for (int i = 0; i < PROCESS_NUM; i++) {
    waitpid(pids[i], NULL, 0);
  }

  for (int i = 0; i < QUEUE_NUM; i++) {
    if (create_message_queue(&mqs[i], mq_names[i], 10, sizeof(toy_msg_t)) < 0) {
      perror("create_message_queue error");
      exit(-1);
    }
  }

  return 0;
}
