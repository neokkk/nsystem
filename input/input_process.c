#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <input_process.h>
#include <common_mq.h>

#define THREAD_NUM 2
#define ARGS_BUF_SIZE 16
#define TOKEN_DELIM " \t\n\r"

#define DEVICE_DRIVER "/dev/n_gpio_driver"

#define ERR_UNKNOWN_COMMAND -2

static mqd_t mqds[MQ_NUM];
static pthread_t tids[THREAD_NUM];

static void *(*thread_funcs[THREAD_NUM])(void *) = {
    command_thread,
    sensor_thread,
};

char *commands[] = {
    "busy",
    "dump",
    "elf",
    "exit",
    "gpio",
    "mincore",
    "mq",
	"mu",
	"send",
    "sh",
};

int (*command_funcs[])(char **args) = {
    &command_busy,
    &command_dump,
    &command_elf,
    &command_exit,
    &command_gpio,
    &command_mincore,
    &command_mq,
    &command_mu,
    &command_send,
    &command_sh,
};

int commands_num() {
    return sizeof(commands) / sizeof(char *);
}

int command_busy(char **argv) {
    printf("command busy\n");
    while (1);
    return -1;
}

int command_dump(char **argv) {
    printf("command dump\n");
    return 0;
}

int command_elf(char **argv) {
    printf("command elf\n");
    return 0;
}

int command_exit(char **argv) {
    printf("command exit\n");
    exit(0);
}

int command_gpio(char **argv) {
    int fd, option = 1;

    printf("command gpio\n");

    if (argv[0] != NULL) {
        option = atoi(argv[0]);
    }

    printf("input option: %d\n", option);

    if ((fd = open(DEVICE_DRIVER, O_RDWR | O_NONBLOCK)) < 0) {
        perror("fail to open n_gpio driver");
        exit(1);
    }

    if (write(fd, (void *)&option, 1) < 0) {
        perror("fail to write to n_gpio driver");
        exit(1);
    }

    close(fd);

    return 0;
}

int command_mincore(char **argv) {
    printf("command mincore\n");
    return 0;
}

int command_mu(char **argv) {
    printf("command mu\n");
    return 0;
}

int command_mq(char **argv) {
    int mqretcode;
    common_msg_t msg;

    printf("command mq\n");

    if (argv[0] == NULL) return -1;

    if (!strcmp(argv[0], "camera")) {
        msg.msg_type = TAKE_PICTURE;
        msg.param1 = 0;
        msg.param2 = 0;

        printf("send: %d\n", mqds[CAMERA_QUEUE]);
        mqretcode = mq_send(mqds[CAMERA_QUEUE], (const char *)&msg, sizeof(msg), 0);
        assert(mqretcode == 0);
    }

    return 0;
}

int command_send(char **argv) {
    printf("command send\n");
    return 0;
}

int command_sh(char **argv) {
    int status;
    pid_t pid;

    printf("command sh\n");

    switch (pid = fork()) {
        case -1:
            perror("fail to fork shell");
            break;
        case 0:
            execvp("sh", argv);
            exit(0);
        default:
            do waitpid(pid, &status, WUNTRACED);
            while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 0;
}

int execute_command(char *command, char** argv) {
    for (int i = 0; i < commands_num(); i++) {
        if (strcmp(command, commands[i]) != 0) continue;
        return command_funcs[i](argv);
    }

    return ERR_UNKNOWN_COMMAND;
}

char **parse_args(char *args) {
    int buf_size = ARGS_BUF_SIZE, position = 0;
    char **buf = (char *)malloc(sizeof(char *) * buf_size), **buf_backup;
    char *token;

    token = strtok(args, TOKEN_DELIM);
    while (token) {
        buf[position] = token;
        position++;

        if (position >= buf_size) {
            buf_size += ARGS_BUF_SIZE;
            buf_backup = buf;
            buf = realloc(buf, sizeof(char *) * buf_size);

            if (!buf) {
                free(buf_backup);
                perror("fail to realloc for args buffer");
                exit(1);
            }
        }

        token = strtok(NULL, TOKEN_DELIM);
    }

    buf[position] = NULL;
    return buf;
}

void *sensor_thread(void *args)
{
    printf("[Sensor] thread created\n");
}

void *command_thread(void *args)
{
	char *line;
    char **argv;
    int result;
	size_t len = 0;

    printf("[Command] thread created\n");

    while (1) {
        printf("INPUT> ");
    
        if (getline(&line, &len, stdin) < 0) {
            perror("fail to read from stdin");
            exit(1);
        }
        if (strcmp(line, "\n") == 0) continue;

        argv = parse_args(line);
        result = execute_command(argv[0], argv + 1);

        if (result < 0) {
            if (result == ERR_UNKNOWN_COMMAND) {
                printf("unknown command\n");
                continue;
            }
            printf("fail to execute %s\n", argv[0]);
            continue;
        }
    }

    free(line);
    free(argv);
}

void init_input_process()
{
	printf("input_process(%d) created\n", getpid());

    mqds[CAMERA_QUEUE] = open_mq("/camera_queue");

	for (int i = 0; i < THREAD_NUM; i++) {
		pthread_create(&tids[i], NULL, thread_funcs[i], (void *)i);
        pthread_detach(tids[i]);
	}

    while (1)
        sleep(1);
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