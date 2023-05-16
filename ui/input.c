#include <input.h>

int input()
{
  printf("input process is running!\n");

  while (1) {
    sleep(1);
  }

  exit(0);
}

pid_t create_input()
{
  pid_t input_pid;
  const char *process_name = "input";

  switch (input_pid = fork()) {
    case -1:
      perror("input fork error");
      exit(-1);
    case 0:
      if (prctl(PR_SET_NAME, process_name) < 0) {
        perror("prctl error");
        exit(-1);
      }
      input();
      break;
    default:
      printf("input pid: %d\n", input_pid);
  }

  return input_pid;
}
