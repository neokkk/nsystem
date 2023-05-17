#include <assert.h>
#include <errno.h>
#include <mqueue.h>
#include <signal.h>
#include <sys/wait.h>
#include <input.h>
#include <gui.h>
#include <system_server.h>
#include <web_server.h>

int main();
void sigchld_handler(int sig);
int create_message_queue(mqd_t* msgq_ptr, const char* queue_name, int num_messages, int message_size);