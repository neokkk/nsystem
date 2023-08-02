#ifndef _COMMON_MQ_H
#define _COMMON_MQ_H

#include <mqueue.h>

#define MQ_NUM 4
#define MSG_COUNT 10

static char *mq_names[MQ_NUM] = {"/watchdog_queue", "/monitor_queue", "/disk_queue", "/camera_queue"};

enum {
	WATCHDOG_QUEUE,
	MONITOR_QUEUE,
	DISK_QUEUE,
	CAMERA_QUEUE,
};

enum {
	SENSOR_DATA,
	DUMP_STATE,
	TAKE_PICTURE,
};

typedef struct {
	/*
	** SENSOR_DATA 0
	** DUMP_STATE 1 
	*/
	unsigned int msg_type;
	unsigned int param1;
	unsigned int param2;
	void *param3;
} common_msg_t;

mqd_t create_mq(const char *mq_name, int message_count, int message_size);
mqd_t open_mq(const char *mq_name);

#endif