#include <system_server.h>

int system_server()
{
  printf("system_server process is running\n");

  while (1) {
    sleep(1);
  }

  exit(0);
}

pid_t create_system_server()
{
  pid_t system_pid;
  const char *process_name = "system_server";

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
