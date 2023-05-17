#define _POSIX_C_SOURCE 200809L
// #include <input.h>
#include <gui.h>
#include <system_server.h>
#include <web_server.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

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

int main()
{
  pid_t spid, gpid, ipid, wpid;
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
  printf("시스템 서버를 생성합니다.\n");
  spid = create_system_server();
  printf("웹 서버를 생성합니다.\n");
  wpid = create_web_server();
  printf("입력 프로세스를 생성합니다.\n");
  ipid = create_input();
  printf("GUI를 생성합니다.\n");
  gpid = create_gui();

  waitpid(spid, NULL, 0);
  waitpid(wpid, NULL, 0);
  waitpid(ipid, NULL, 0);
  waitpid(gpid, NULL, 0);

  return 0;
}
