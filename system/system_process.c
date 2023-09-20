#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <mosquitto.h>
#include <mqtt_protocol.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <system_process.h>
#include <dump_state.h>
#include <hardware.h>
#include <sensor_info.h>
#include <system_info.h>
#include <common_mq.h>
#include <common_shm.h>
#include <common_timer.h>

#define THREAD_NUM 4
#define BUF_SIZE 1024

#define MOSQ_HOST "192.168.31.182"
#define MOSQ_PORT 1883
#define MOSQ_TOPIC_SENSOR "sensor"
#define MOSQ_TOPIC_SYSTEM "system"

static mqd_t mqds[MQ_NUM];
static pthread_t tids[THREAD_NUM];
static struct mosquitto *mosq;
static system_info_t system_info;

static void *(*thread_funcs[THREAD_NUM])(void *) = {
	watchdog_thread,
	monitor_thread,
	camera_thread,
	disk_thread,
};

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
		if ((strcmp(dit->d_name, ".") == 0) || (strcmp(dit->d_name, "..") == 0)) continue;
		sprintf(filePath, "%s/%s", dirname, dit->d_name);

		if (lstat(filePath, &st) != 0) continue;
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

int get_sys_info(system_info_t *sys_info)
{
	FILE *fp;
	char *line = NULL, model[SYS_NAME];
	size_t len, cores = 0;
	ssize_t nread;

	if ((fp = fopen("/proc/cpuinfo", "r")) == NULL) {
		perror("fail to open /proc/cpuinfo");
		return -1;
	}

	while ((nread = getline(&line, &len, fp)) != -1) {
		if (strncmp(line, "processor", 9) == 0) {
			cores += 1;
			continue;
		}

		if (strncmp(line, "Model", 5) != 0) continue;
		sscanf(line, "Model : %127[^\n] ", model);
		break;
	}

	sys_info->cores = cores;
	strncpy(sys_info->name, model, strlen(model) + 1);

	fclose(fp);
	free(line);

	return 0;
}

int get_proc_info(proc_info_t *proc_info)
{
	int fd;
	char buf[BUF_SIZE];
	ssize_t nread;

	if ((fd = open("/proc/loadavg", O_RDONLY)) < 0) {
		perror("fail to open /proc/loadavg");
		return -1;
	}

	if ((nread = read(fd, buf, sizeof(buf))) < 0) {
		perror("fail to read /proc/loadavg");
		goto err;
	}

	sscanf(buf, "%f %f %*f %d/%d",
		&proc_info->load_avg_for_1, &proc_info->load_avg_for_5, &proc_info->running, &proc_info->total);

	close(fd);
	return 0;

err:
	close(fd);
	return -1;
}

int get_mem_info(mem_info_t *mem_info)
{
	FILE *fp;
	char *line = NULL;
	size_t len;
	ssize_t nread;

	if ((fp = fopen("/proc/meminfo", "r")) == NULL) {
		perror("fail to read /proc/meminfo");
		return -1;
	}

	while ((nread = getline(&line, &len, fp)) != -1) {
		if (strncmp(line, "MemTotal", 8) == 0) {
			sscanf(line, "MemTotal: %llu ", &mem_info->total);
			continue;
		}
		if (strncmp(line, "MemAvailable", 12) == 0) {
			sscanf(line, "MemAvailable: %llu ", &mem_info->available);
			continue;
		}
		if (strncmp(line, "Active", 6) == 0) {
			sscanf(line, "Active: %llu ", &mem_info->active);
			continue;
		}
		if (strncmp(line, "Inactive", 8) == 0) {
			sscanf(line, "Inactive: %llu ", &mem_info->inactive);
			continue;
		}
		if (strncmp(line, "SwapTotal", 9) == 0) {
			sscanf(line, "SwapTotal: %llu ", &mem_info->swapped);
			continue;
		}
		if (strncmp(line, "Mapped", 6) == 0) {
			sscanf(line, "Mapped: %llu ", &mem_info->mapped);
			break;
		}
	}

	fclose(fp);
	free(line);

	return 0;
}

int get_disk_info(disk_info_t *disk_info)
{
	struct statvfs svfs;

	if (statvfs("/", &svfs) == 0) {
		strncpy(disk_info->mount, "/", 1);
		disk_info->total = (unsigned long long)svfs.f_blocks * svfs.f_frsize / 1024;
		disk_info->used = (unsigned long long)(svfs.f_blocks - svfs.f_bfree) * svfs.f_frsize / 1024;
	}

	return 0;
}

int get_engine_info(engine_info_t *engine_info)
{
	int fd;
	char buf[10];

	if ((fd = open("/sys/kernel/engine/motor_1", O_RDONLY, 0777)) < 0) {
		perror("fail to open motor_1");
		return -1;
	}

	if (read(fd, buf, sizeof(buf)) < 0) {
		perror("fail to read motor_1 speed");
		goto err;
	}

	printf("motor_1 speed: %s\n", buf);
	engine_info->motor_1_speed = atoi(buf);
	close(fd);
	memset(buf, '\0', sizeof(buf));

	if ((fd = open("/sys/kernel/engine/motor_2", O_RDONLY, 0777)) < 0) {
		perror("fail to open motor_2");
		return -1;
	}

	if (read(fd, buf, sizeof(int)) < 0) {
		perror("fail to read motor_2 speed");
		goto err;
	}

	printf("motor_2 speed: %s\n", buf);
	engine_info->motor_2_speed = atoi(buf);
	close(fd);

	return 0;

err:
	close(fd);
	return -1;
}

int get_system_info()
{
	get_sys_info(&system_info);
	get_proc_info(&system_info.process);
	get_mem_info(&system_info.memory);
	get_disk_info(&system_info.disk);
	get_engine_info(&system_info.engine);
	return 0;
}

void stringify_system_info(char *buf)
{
	sprintf(buf, 
		"{\"name\":\"%s\",\"cores\":%d,\"engine\":{\"motor_1_speed\":%d,\"motor_2_speed\":%d},\"process\":{\"total\":%d,\"running\":%d,\"load_avg_for_1\":%f,\"load_avg_for_5\":%f},\"memory\":{\"total\":%llu,\"available\":%llu,\"active\":%llu,\"inactive\":%llu,\"swapped\":%llu,\"mapped\":%llu},\"disk\":{\"mount\":\"%s\",\"total\":%llu,\"used\":%llu}}",
		system_info.name, system_info.cores,
		system_info.engine.motor_1_speed, system_info.engine.motor_2_speed,
		system_info.process.total, system_info.process.running, system_info.process.load_avg_for_1, system_info.process.load_avg_for_5,
		system_info.memory.total, system_info.memory.available, system_info.memory.active, system_info.memory.inactive, system_info.memory.swapped, system_info.memory.mapped,
		system_info.disk.mount, system_info.disk.total, system_info.disk.used);
}

/*
 * 파일 이벤트 수신 스레드
*/
void *disk_thread(void *arg)
{
	printf("[Disk] created thread\n");
}

/*
 * 카메라 이벤트 수신 스레드
*/
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

void mosq_disconnect_callback(struct mosquitto *_mosq, void *userdata, int result, const mosquitto_property *props)
{
	printf("[Monitor] mosquitto session disconnected %d\n", result);
}

/*
 * 센서 데이터 수신 스레드
*/
void *monitor_thread(void *arg)
{
	int shm_fd, retcode;
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
				printf("[Monitor] temperature: %f°C, pressure: %fhPa\n", bmp_info->temp, bmp_info->press);
				retcode = mosquitto_publish(mosq, NULL, MOSQ_TOPIC_SENSOR, sizeof(sensor_info_t), bmp_info, 0, false);
				if (retcode) {
					perror("[Monitor] fail to publish topic");
				}
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


void sigalrm_handler(int sig)
{
	char buf[4096];
	int retcode;
	printf("[Watchdog] caught SIGALRM\n");
	get_system_info();
	stringify_system_info(buf);
	retcode = mosquitto_publish(mosq, NULL, MOSQ_TOPIC_SYSTEM, strlen(buf), buf, 0, false);
	if (retcode) {
		perror("[Watchdog] fail to publish topic");
	}
}

/*
 * 시스템 정보 수집 스레드
*/
void *watchdog_thread(void *arg)
{
	int retcode;
	common_msg_t msg;

	printf("[Watchdog] created thread\n");
	
	signal(SIGALRM, sigalrm_handler);
	set_periodic_timer(5, 0);
}

void init_system_process()
{
	int i, retcode;

	printf("system_process(%d) created\n", getpid());
	mosq = mosquitto_new(NULL, true, NULL);

	if (!mosq) {
		perror("fail to create session");
		goto err;
	}

	retcode = mosquitto_connect(mosq, MOSQ_HOST, MOSQ_PORT, 0);
	if (retcode) {
		perror("fail to connect to mqtt broker");
		goto err;
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

err:
	mosquitto_disconnect(mosq);
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