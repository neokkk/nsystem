#include <gui.h>

pid_t create_gui()
{
  pid_t gui_pid;
  const char* process_name = "gui";

  printf("GUI를 생성합니다.\n");

  switch (gui_pid = fork()) {
    case -1:
      perror("gui fork error");
      exit(-1);
    case 0:
      if (prctl(PR_SET_NAME, process_name) < 0) {
        perror("prctl error");
        exit(-1);
      }
      execl("/usr/bin/google-chrome-stable", "google-chrome-stable", "--disable-gpu", "http://localhost:8888", NULL);
      break;
    default:
      printf("gui pid: %d\n", gui_pid);
  }

  return gui_pid;
}
