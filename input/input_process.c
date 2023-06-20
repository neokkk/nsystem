#include <input_process.h>

void init_input_process()
{
	printf("input_process(%d) created\n", getpid());
}

pid_t create_input_process()
{
	pid_t pid;
	const char *process_name = "input";

	switch (pid = fork()) {
		case -1:
			perror("fail to fork input_process");
			exit(1);
		case 0:
			if (prctl(PR_SET_NAME, process_name) < 0) {
				perror("fail to change input_process name");
				exit(1);
			}
			init_input_process();
			exit(0);
		default:
			break;
	}

	return pid;
}