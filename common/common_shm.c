#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <common_shm.h>

int create_shm(const char *shm_name)
{
	int fd;

	fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (fd <  0) {
		printf("existed shared memory\n");
		if ((fd = shm_open(shm_name, O_RDWR, 0666)) < 0) return -1;
		return 0;
	}

	printf("%s is created\n", shm_name);
	return fd;
}

int open_shm(const char *shm_name)
{
	int fd;

	if ((fd = shm_open(shm_name, O_RDWR, 0666)) < 0) return -1;
	printf("%s is opened\n", shm_name);
	return fd;
}

void close_shm(int fd)
{
	close(fd);
}