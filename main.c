#include <errno.h>
#include <mosquitto.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <input_process.h>
#include <system_process.h>
#include <common_mq.h>
#include <common_shm.h>
#include <sensor_info.h>

#define PROCESS_NUM 2

static mqd_t mqds[MQ_NUM];
static pid_t pids[PROCESS_NUM];

static pid_t (*p_funcs[])() = {
	create_system_process,
	create_input_process,
};

void sigchld_handler(int sig)
{
	int status, saved_errno;
	pid_t child_pid;

	saved_errno = errno;

	while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
		printf("SIGCHLD(%d) caught\n", child_pid);
	}

	if (child_pid < 0 && errno != ECHILD) {
		perror("fail to wait child process");
		exit(1);
	}

	errno = saved_errno;
}

int main()
{
	int i, bmp_shm_fd;

	signal(SIGCHLD, sigchld_handler);

	bmp_shm_fd = create_shm(shm_names[BMP280]);
	ftruncate(bmp_shm_fd, sizeof(sensor_info_t));
	mosquitto_lib_init();

	for (i = 0; i < MQ_NUM; i++) {
		mqds[i] = create_mq(mq_names[i], MSG_COUNT, sizeof(common_msg_t));
	}

	for (i = 0; i < PROCESS_NUM; i++) {
		pids[i] = p_funcs[i]();
	}

	for (i = 0; i < PROCESS_NUM; i++) {
		if (waitpid(pids[i], NULL, 0) < 0) {
			perror("fail to wait child process");
			goto err;
		}
	}

	for (i = 0; i < MQ_NUM; i++) {
		close_mq(mq_names[i]);
	}
	mosquitto_lib_cleanup();
	close_shm(shm_names[BMP280]);
	return 0;

err:
	for (i = 0; i < MQ_NUM; i++) {
		close_mq(mq_names[i]);
	}
	mosquitto_lib_cleanup();
	close_shm(shm_names[BMP280]);
	return 1;
}