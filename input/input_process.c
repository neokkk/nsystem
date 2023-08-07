#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <input_process.h>
#include <sensor_info.h>
#include <common_mq.h>
#include <common_shm.h>

#define THREAD_NUM 2
#define POLLFD_NUM 1
#define ARGS_BUF_SIZE 16
#define BUF_SIZE 1024
#define TOKEN_DELIM " \t\n\r"

#define ENGINE_DRIVER "/dev/engine_driver"
#define SIMPLE_IO_DRIVER "/dev/simple_io_driver"
#define SENSOR_SYSFS_NOTYFY "/sys/kernel/bmp280/notify"

#define MOTOR_1_SET_SPEED _IOW('w', '1', int32_t *)
#define MOTOR_2_SET_SPEED _IOW('w', '2', int32_t *)

#define ERR_UNKNOWN_COMMAND -2

static mqd_t mqds[MQ_NUM];
static pthread_t tids[THREAD_NUM];
static sensor_info_t *bmp_info;
static int bmp_shm_fd;

static void *(*thread_funcs[THREAD_NUM])(void *) = {
    command_thread,
    sensor_thread,
};

char *commands[] = {
    "busy",
    "dump",
    "elf",
    "exit",
    "m1",
    "m2",
    "mincore",
    "mq",
	"mu",
    "sh",
    "sio",
};

int (*command_funcs[])(char **args) = {
    &command_busy,
    &command_dump,
    &command_elf,
    &command_exit,
    &command_set_motor_1_speed,
    &command_set_motor_2_speed,
    &command_mincore,
    &command_mq,
    &command_mu,
    &command_sh,
    &command_simple_io,
};

int commands_len = sizeof(commands) / sizeof(char *);

int command_busy(char **argv)
{
    printf("command busy\n");
    while (1);
    return -1;
}

int command_dump(char **argv)
{
    int mqretcode;
    common_msg_t msg;

    printf("command dump\n");

    msg.msg_type = DUMP_STATE;
    msg.param1 = 0;
    msg.param2 = 0;

    mqretcode = mq_send(mqds[MONITOR_QUEUE], (const char *)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);

    return 0;
}

int command_elf(char **argv)
{
    printf("command elf\n");

    int fd;
    char *contents = NULL;
    size_t contents_len;
    struct stat st;
    Elf64Hdr *map;

    fd = open("./sample/sample.elf", O_RDONLY);
	if (fd < 0) {
        printf("fail to open sample.elf\n");
        return -1;
    }

    if (!fstat(fd, &st)) {
        contents_len = st.st_size;
        if (!contents_len) {
            printf("empty file!");
            goto err;
        }
        printf("File size: %ld\n", contents_len);
        map = (Elf64Hdr *)mmap(NULL, contents_len, PROT_READ, MAP_PRIVATE, fd, 0);
        printf("Object file type: %d\n", map->e_type);
        printf("Architecture: %d\n", map->e_machine);
        printf("Object file version: %d\n", map->e_version);
        printf("Entry point virtual address: %ld\n", map->e_entry);
        printf("Program header table file offset: %ld\n", map->e_phoff);
        munmap(map, contents_len);
    }

    return 0;

err:
    close(fd);
    return -1;
}

int command_exit(char **argv)
{
    printf("command exit\n");
    exit(0);
}

int command_set_motor_1_speed(char **argv)
{
    int fd, speed;

    if (argv[0] == NULL) return -1;

    speed = atoi(argv[0]);
    printf("speed: %d\n", speed);

    if ((fd = open(ENGINE_DRIVER, O_RDWR | O_NDELAY)) < 0) {
        perror("fail to open engine driver");
        return -1;
    }

    if (ioctl(fd, MOTOR_1_SET_SPEED, speed) < 0) {
        perror("fail to set speed of motor 1");
        goto err;
    }

    close(fd);
    return 0;

err:
    close(fd);
    return -1;
}

int command_set_motor_2_speed(char **argv)
{
    int fd, speed;

    if (argv[0] == NULL) return -1;

    speed = atoi(argv[0]);
    printf("speed: %d\n", speed);

    if ((fd = open(ENGINE_DRIVER, O_RDWR | O_NDELAY)) < 0) {
        perror("fail to open engine driver");
        return -1;
    }

    if (ioctl(fd, MOTOR_2_SET_SPEED, speed) < 0) {
        perror("fail to set speed of motor 2");
        goto err;
    }

    close(fd);
    return 0;

err:
    close(fd);
    return -1;
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

        if (argv[1]) {
            msg.msg_type = atoi(argv[1]);
        }

        mqretcode = mq_send(mqds[CAMERA_QUEUE], (const char *)&msg, sizeof(msg), 0);
        if (mqretcode != 0) {
            perror("fail to send message to camera thread");
        }
    }

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

int command_simple_io(char **argv)
{
    int fd, option = 1;

    printf("command simple_io\n");

    if (argv[0] != NULL) {
        option = atoi(argv[0]);
    }

    printf("input option: %d\n", option);

    if ((fd = open(SIMPLE_IO_DRIVER, O_RDWR | O_NONBLOCK)) < 0) {
        perror("fail to open simple_io driver");
        goto err;
    }

    if (write(fd, (void *)&option, 1) < 0) {
        perror("fail to write to simple_io driver");
        goto err;
    }

    close(fd);
    return 0;

err:
    close(fd);
    exit(1);
}

int execute_command(char *command, char** argv) {
    for (int i = 0; i < commands_len; i++) {
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
    int fd, nread, retfdnum, mqretcode;
    float temp, press;
    char buf[BUF_SIZE];
    struct pollfd pfds[POLLFD_NUM];
    common_msg_t msg;

    printf("[Sensor] created thread\n");

    if ((fd = open(SENSOR_SYSFS_NOTYFY, O_RDONLY | O_CLOEXEC | O_NONBLOCK)) < 0) {
        perror("[Sensor] fail to open device driver");
        exit(1);
    }

    pfds[0].fd = fd;
    pfds[0].events = POLLIN;

    bmp_shm_fd = open_shm(shm_names[BMP280]);
    bmp_info = (sensor_info_t *)mmap(NULL, sizeof(sensor_info_t), PROT_READ | PROT_WRITE, MAP_SHARED, bmp_shm_fd, 0);

    msg.msg_type = SENSOR_DATA;

    while (1) {
        retfdnum = poll(pfds, POLLFD_NUM, -1);
        if (retfdnum == 0) continue;
        if (retfdnum < 0) {
            perror("[Sensor] fail to poll from device driver");
            goto err;
        }

        memset(buf, 0, sizeof(buf));
        nread = read(fd, buf, sizeof(buf));
        if (nread == 0) continue;
        if (nread < 0) {
            perror("[Sensor] fail to read from device driver");
            goto err;
        }

        lseek(fd, 0, SEEK_SET);
        sscanf(buf, "%f %f", &temp, &press);

        bmp_info->temp = temp;
        bmp_info->press = press;

        msg.param1 = 0;
        msg.param2 = 0;

        mqretcode = mq_send(mqds[MONITOR_QUEUE], (const char *)&msg, sizeof(msg), 0);
        if (mqretcode < 0) {
            perror("[Sensor] fail to send to monitor thread");
        }
        sleep(10);
    }

err:
    munmap(bmp_info, sizeof(sensor_info_t));
    shm_unlink(shm_names[BMP280]);
    close(bmp_shm_fd);
    close(fd);
    exit(1);
}

void *command_thread(void *args)
{
	char *line;
    char **argv;
    int result;
	size_t len = 0;

    printf("[Command] created thread\n");

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
    int i;

	printf("input_process(%d) created\n", getpid());

    if (bmp_info == MAP_FAILED) {
        perror("fail to allocate shared memory");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < MQ_NUM; i++) {
        mqds[i] = open_mq(mq_names[i]);
    }

	for (i = 0; i < THREAD_NUM; i++) {
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