#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <common_mq.h>

mqd_t create_mq(const char *mq_name, int message_count, int message_size)
{
    mqd_t mq_tmp;
    struct mq_attr mq_attr;

    memset(&mq_attr, 0, sizeof(mq_attr));
    mq_attr.mq_msgsize = message_size;
    mq_attr.mq_maxmsg = message_count;

    mq_unlink(mq_name);
    mq_tmp = mq_open(mq_name, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0777, &mq_attr);

    if (mq_tmp < 0) {
		printf("existed %s\n", mq_name);
        if ((mq_tmp = mq_open(mq_name, O_RDWR)) < 0) return -1;
        return 0;
    }

	printf("%s is created\n", mq_name);
    return mq_tmp;
}

mqd_t open_mq(const char *mq_name)
{
    mqd_t mq_tmp;

    if ((mq_tmp = mq_open(mq_name, O_RDWR)) < 0) return -1;

	printf("%s is opened\n", mq_name);
    return mq_tmp;
}

int close_mq(mqd_t mqd) {
    if (mq_close(mqd) < 0) return -1;
    return 0;
}