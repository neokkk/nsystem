#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <system_process.h>
#include <dump_state.h>
#include <hardware.h>
#include <sensor_info.h>
#include <common_mq.h>
#include <common_shm.h>
#include <common_timer.h>

#define THREAD_NUM 5

static mqd_t mqds[MQ_NUM];
static pthread_t tids[THREAD_NUM];

static void *(*thread_funcs[THREAD_NUM])(void *) = {
	watchdog_thread,
	monitor_thread,
	camera_thread,
	disk_thread,
	timer_thread,
};

void sigalrm_handler(int sig)
{
	// printf("[Timer] caught SIGALRM\n");
}

/*
 * 인터벌 타이머 스레드
*/
void *timer_thread(void *arg)
{
	printf("[Timer] created thread\n");

	signal(SIGALRM, sigalrm_handler);
	set_periodic_timer(1, 0);
}

long get_directory_size(char *dirname)
{
	DIR *dir = opendir(dirname);
	if (dir == 0)
		return 0;

	struct dirent *dit;
	struct stat st;
	long size = 0;
	long total_size = 0;
	char filePath[1024];

	while ((dit = readdir(dir)) != NULL) {
		if ((strcmp(dit->d_name, ".") == 0) || (strcmp(dit->d_name, "..") == 0))
			continue;

		sprintf(filePath, "%s/%s", dirname, dit->d_name);
		if (lstat(filePath, &st) != 0)
			continue;
		size = st.st_size;

		if (S_ISDIR(st.st_mode)) {
			long dir_size = get_directory_size(filePath) + size;
			total_size += dir_size;
		} else {
			total_size += size;
		}
	}
	return total_size;
}

/*
 * 파일 이벤트 수신 스레드
*/
void *disk_thread(void *arg)
{
	printf("[Disk] created thread\n");
}

void *camera_thread(void *arg)
{
	int mqretcode, halretcode;
	common_msg_t msg;
	hw_module_t *module;

	printf("[Camera] created thread\n");

	halretcode = get_camera_module((const hw_module_t **)&module);
	assert(halretcode == 0);

	while (1) {
		mqretcode = mq_receive(mqds[CAMERA_QUEUE], (void *)&msg, sizeof(msg), 0);
		assert(mqretcode >= 0);
		printf("[Camera] received message_type: %d, param1: %d, param2: %d\n", msg.msg_type, msg.param1, msg.param2);

		switch (msg.msg_type) {
			case TAKE_PICTURE:
				module->take_picture();
				break;
			default:
				printf("[Camera] unknown message type\n");
		}
	}
}

/*
 * 센서 데이터 수신 스레드
*/
void *monitor_thread(void *arg)
{
	int shm_fd;
	common_msg_t msg;
	sensor_info_t *bmp_info;

	printf("[Monitor] created thread\n");

	shm_fd = open_shm(shm_names[BMP280]);
	if (shm_fd < 0) {
		perror("[Monitor] fail to open shared memory");
		exit(EXIT_FAILURE);
	}

	bmp_info = (sensor_info_t *)mmap(NULL, sizeof(sensor_info_t), PROT_READ, MAP_SHARED, shm_fd, 0);
	if (bmp_info == MAP_FAILED) {
		perror("[Monitor] fail to allocate shared memory");
		goto err;
	}

	while (1) {
		if ((int)mq_receive(mqds[MONITOR_QUEUE], (void *)&msg, sizeof(msg), 0) < 0) {
			perror("[Monitor] fail to receive message");
			goto err;
		}

		switch (msg.msg_type) {
			case SENSOR_DATA:
				printf("[Monitor] temperature: %f°C\n", bmp_info->temp);
				printf("[Monitor] pressure: %fhPa\n", bmp_info->press);
				break;
			case DUMP_STATE:
				dump_state();
				break;
			default:
				perror("[Monitor] unknown message type\n");
		}
	}

err:
	shm_unlink(shm_names[BMP280]);
	munmap(shm_fd, sizeof(sensor_info_t));
	exit(EXIT_FAILURE);
}

/*
 * 시스템 정보 수집 스레드
*/
void *watchdog_thread(void *arg)
{
	common_msg_t msg;

	printf("[Watchdog] created thread\n");
	
	while (1) {
		if ((int)mq_receive(mqds[WATCHDOG_QUEUE], (void *)&msg, sizeof(msg), 0) < 0) {
			perror("[Watchdog] fail to receive message");
			exit(1);
		}

		printf("[Watchdog] recieved message_type: %d, param1: %d, param2: %d\n",
			msg.msg_type, msg.param1, msg.param2);
	}
}

void init_system_process()
{
	int i;

	printf("system_process(%d) created\n", getpid());

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

pid_t create_system_process()
{
	pid_t pid;
	const char *process_name = "system";

	switch (pid = fork()) {
		case -1:
			perror("fail to fork system_process");
			exit(1);
		case 0:
			if (prctl(PR_SET_NAME, process_name) < 0) {
				perror("fail to change system_process name");
				exit(1);
			}
			init_system_process();
			exit(0);
		default:
			break;
	}

	return pid;
}