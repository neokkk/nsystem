#include <web_server.h>

pid_t create_web_server() {
  pid_t web_pid;
  const char* process_name = "web_server";

  printf("웹 서버를 생성합니다.\n");

  switch (web_pid = fork()) {
    case -1:
      perror("web_server fork error");
      exit(-1);
    case 0:
      if (prctl(PR_SET_NAME, process_name) < 0) {
        perror("prctl error");
        exit(-1);
      }
      execl("/usr/local/bin/filebrowser", "filebrowser", "-p", "8888", NULL);
      break;
    default:
      printf("web_server pid: %d\n", web_pid);
  }

  return web_pid;
}
