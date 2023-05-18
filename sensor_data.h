#ifndef _SENSOR_DATA_H_
#define _SENSOR_DATA_H_

#define SHM_SENSOR_KEY 0x1000

typedef struct {
  int humidity;
  int pressure;
  int temperature;
} sensor_data_t;

#endif