#include <input.h>
#include <gui.h>
#include <system_server.h>
#include <web_server.h>
#include <signal.h>
#include <sys/wait.h>

void sigchld_handler(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main()
{
  pid_t spid, gpid, ipid, wpid;
  int status, saved_errno;
  int sig_cnt;
  sigset_t block_mask, empty_mask;
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

  waitpid(ipid, NULL, 0);
  waitpid(spid, NULL, 0);
  waitpid(wpid, NULL, 0);
  waitpid(gpid, NULL, 0);

  return 0;
}
