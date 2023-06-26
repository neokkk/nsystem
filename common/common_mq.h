#ifndef _COMMON_MQ_H
#define _COMMON_MQ_H

#include <mqueue.h>

#define MQ_NUM 4

#define SENSOR_DATA 0
#define DUMP_STATE 1

static mqd_t mqds[MQ_NUM];
static char *mq_names[MQ_NUM] = {"/watchdog_queue", "/monitor_queue", "/disk_queue", "/camera_queue"};

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

int open_mq(mqd_t *mq, const char *queue_name, int message_count, int message_size);

#endif