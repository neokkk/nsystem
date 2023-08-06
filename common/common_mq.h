#ifndef _COMMON_MQ_H_
#define _COMMON_MQ_H_

#include <mqueue.h>
#include <stdlib.h>

#define MSG_COUNT 10

enum queue_type {
	MQ_BASE = -1,
	WATCHDOG_QUEUE,
	MONITOR_QUEUE,
	DISK_QUEUE,
	CAMERA_QUEUE,
	MQ_MAX,
};
#define MQ_NUM (MQ_MAX - (MQ_BASE) - 1)

enum msg_type {
	SENSOR_DATA,
	DUMP_STATE,
	TAKE_PICTURE,
};

typedef struct {
	unsigned int msg_type;
	unsigned int param1;
	unsigned int param2;
} common_msg_t;

static char *mq_names[] = {"/watchdog_queue", "/monitor_queue", "/disk_queue", "/camera_queue"};

mqd_t create_mq(const char *mq_name, int message_count, int message_size);
mqd_t open_mq(const char *mq_name);
int close_mq(mqd_t mqd);

#endif