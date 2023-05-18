#define SHM_SENSOR_KEY 0x1000

typedef struct sensor_data {
  int humidity;
  int pressure;
  int temperature;
} sensor_data_t;
