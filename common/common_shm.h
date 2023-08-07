#ifndef _COMMON_SHM_H_
#define _COMMON_SHM_H_

enum shm_type {
  SENSOR_BASE = -1,
  BMP280,
  SENSOR_MAX,
};
#define SHM_NUM (SENSOR_MAX - SENSOR_BASE - 1)

static char *shm_names[] = {"bmp280"};

int create_shm(const char *shm_name);
int open_shm(const char *shm_name);
void close_shm(int fd);

#endif
