#include <web_process.h>

void init_web_process()
{
	printf("web_process(%d) created\n", getpid());
}

pid_t create_web_process()
{
	pid_t pid;
	const char *process_name = "web";

	switch (pid = fork()) {
		case -1:
			perror("fail to fork web_process");
			exit(1);
		case 0:
			if (prctl(PR_SET_NAME, process_name) < 0) {
				perror("fail to change web_process name");
				exit(1);
			}
			init_web_process();
			exit(0);
		default:
			break;
	}

	return pid;
}