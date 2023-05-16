#include <gui.h>
#include <input.h>
#include <system_server.h>
#include <web_server.h>
#include <sys/wait.h>

int main()
{
  pid_t i_pid = create_input();
  pid_t ss_pid = create_system_server();
  pid_t ws_pid = create_web_server();
  pid_t g_pid = create_gui();

  waitpid(i_pid, NULL, 0);
  waitpid(ss_pid, NULL, 0);
  waitpid(ws_pid, NULL, 0);
  waitpid(g_pid, NULL, 0);

  return 0;
}
