#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <assert.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>
#include <toy_message.h>

#define NUM_MESSAGES 10

static mqd_t watchdog_queue;
static mqd_t monitor_queue;
static mqd_t disk_queue;
static mqd_t camera_queue;

static void sigchld_handler(int sig) {
  int status, saved_errno;
  pid_t child_pid;

  saved_errno = errno;

  printf("handler: Caught SIGCHLD : %d\n", sig);

  while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
    printf("handler: Reaped child %ld - ", (long)child_pid);
    (NULL, status);
  }

  if (child_pid == -1 && errno != ECHILD)
    printf("waitpid");

  printf("handler: returning\n");

  errno = saved_errno;
}

int create_message_queue(mqd_t *msgq_ptr, const char *queue_name, int num_messages, int message_size) {
  struct mq_attr mq_attrib;
  int mq_errno;
  mqd_t msgq;

  printf("%s name=%s nummsgs=%d\n", __func__, queue_name, num_messages);

  memset(&mq_attrib, 0, sizeof(mq_attrib));
  mq_attrib.mq_msgsize = message_size;
  mq_attrib.mq_maxmsg = num_messages;

  mq_unlink(queue_name);
  msgq = mq_open(queue_name, O_RDWR | O_CREAT | O_CLOEXEC, 0777, &mq_attrib);

  if (msgq == -1) {
    printf("%s queue=%s already exists so try to open\n", __func__, queue_name);
    msgq = mq_open(queue_name, O_RDWR);
    assert(msgq != (mqd_t) -1);
    printf("%s queue=%s opened successfully\n", __func__, queue_name);
    return -1;
  }

  *msgq_ptr = msgq;
  return 0;
}

int main() {
  pid_t spid, gpid, ipid, wpid;
  int status, saved_errno;
  struct sigaction sa;
  int retcode;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = sigchld_handler;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    printf("sigaction");
    return 0;
  }

  printf("메인 함수입니다.\n");

  retcode = create_message_queue(&watchdog_queue, "/watchdog_queue", NUM_MESSAGES, sizeof(toy_msg_t));
  assert(retcode == 0);
  retcode = create_message_queue(&monitor_queue, "/monitor_queue", NUM_MESSAGES, sizeof(toy_msg_t));
  assert(retcode == 0);
  retcode = create_message_queue(&disk_queue, "/disk_queue", NUM_MESSAGES, sizeof(toy_msg_t));
  assert(retcode == 0);
  retcode = create_message_queue(&camera_queue, "/camera_queue", NUM_MESSAGES, sizeof(toy_msg_t));
  assert(retcode == 0);

  spid = create_system_server();
  wpid = create_web_server();
  ipid = create_input();
  gpid = create_gui();

  waitpid(spid, &status, 0);
  waitpid(gpid, &status, 0);
  waitpid(ipid, &status, 0);
  waitpid(wpid, &status, 0);

  return 0;
}
