#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <system_process.h>
#include <common_mq.h>
#include <common_timer.h>

#define MESSAGE_COUNT 10
#define THREAD_NUM 5

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
	printf("timer thread created\n");

	signal(SIGALRM, sigalrm_handler);
	set_periodic_timer(1, 0);
}

/*
 * 파일 이벤트 수신 스레드
*/
void *disk_thread(void *arg)
{
	printf("disk thread created\n");
}

void *camera_thread(void *arg)
{
	printf("camera thread created\n");
}

/*
 * 센서 데이터 모니터링 스레드
*/
void *monitor_thread(void *arg)
{
	common_msg_t msg;

	printf("monitor thread created\n");

	while (1) {
		if ((int)mq_receive(mqds[1], (void *)&msg, sizeof(msg), 0) < 0) {
			perror("fail to receive monitor message");
			exit(1);
		}

		printf("monitor received message_type: %d, param1: %d, param2: %d, param3: %d\n", msg.msg_type, msg.param1, msg.param2, (int)msg.param3);

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

	printf("watchdog thread created\n");
	
	while (1) {
		if ((int)mq_receive(mqds[0], (void *)&msg, sizeof(msg), 0) < 0) {
			perror("fail to receive watchdog message");
			exit(1);
		}

		printf("watchdog recieved message_type: %d, param1: %d, param2: %d, param3: %d\n", msg.msg_type, msg.param1, msg.param2, (int)msg.param3);
	}
}

void init_system_process()
{
	printf("system_process(%d) created\n", getpid());

	for (int i = 0; i < MQ_NUM; i++) {
		if (open_mq(&mqds[i], mq_names[i], MESSAGE_COUNT, sizeof(common_msg_t)) < 0) {
			perror("fail to open message queue");
			exit(1);
		}
	}

	for (int i = 0; i < THREAD_NUM; i++) {
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