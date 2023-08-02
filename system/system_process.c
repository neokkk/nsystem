#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <system_process.h>
#include <hardware.h>
#include <common_mq.h>
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
	// printf("SIGALRM caught\n");
}

/*
 * 인터벌 타이머 스레드
*/
void *timer_thread(void *arg)
{
	printf("[Timer] thread created\n");

	signal(SIGALRM, sigalrm_handler);
	set_periodic_timer(1, 0);
}

/*
 * 파일 이벤트 수신 스레드
*/
void *disk_thread(void *arg)
{
	printf("[Disk] thread created\n");
}

void *camera_thread(void *arg)
{
	int mqretcode, halretcode;
	common_msg_t msg;
	hw_module_t *module;

	printf("[Camera] thread created\n");

	halretcode = get_camera_module((const hw_module_t **)&module);
	assert(halretcode == 0);

	while (1) {
		mqretcode = mq_receive(mqds[CAMERA_QUEUE], (void *)&msg, sizeof(msg), 0);
		assert(mqretcode >= 0);
		printf("[Camera] received message_type: %d, param1: %d, param2: %d\n", msg.msg_type, msg.param1, msg.param2);

		switch (msg.msg_type) {
			case TAKE_PICTURE:
				printf("[Camera] take picture!\n");
				module->take_picture();
				break;
			case DUMP_STATE:
				printf("[Camera] dump state\n");
				module->dump();
				break;
			default:
				printf("[Camera] unknown message type\n");
		}
	}
}

/*
 * 센서 데이터 모니터링 스레드
*/
void *monitor_thread(void *arg)
{
	common_msg_t msg;

	printf("[Monitor] thread created\n");

	while (1) {
		if ((int)mq_receive(mqds[1], (void *)&msg, sizeof(msg), 0) < 0) {
			perror("fail to receive monitor message");
			exit(1);
		}

		printf("[Monitor] received message_type: %d, param1: %d, param2: %d, param3: %d\n", msg.msg_type, msg.param1, msg.param2, (int)msg.param3);

		switch (msg.msg_type) {
			case SENSOR_DATA:
				break;
			case DUMP_STATE:
				break;
			default:
				perror("unknown message type\n");
		}
	}
}

/*
 * 메시지 수신 스레드
*/
void *watchdog_thread(void *arg)
{
	common_msg_t msg;

	printf("[Watchdog] thread created\n");
	
	while (1) {
		if ((int)mq_receive(mqds[WATCHDOG_QUEUE], (void *)&msg, sizeof(msg), 0) < 0) {
			perror("fail to receive watchdog message");
			exit(1);
		}

		printf("[Watchdog] recieved message_type: %d, param1: %d, param2: %d\n", msg.msg_type, msg.param1, msg.param2);
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