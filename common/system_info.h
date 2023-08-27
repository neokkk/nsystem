#ifndef _SYSTEM_INFO_H_
#define _SYSTEM_INFO_H_

#define SYS_NAME 128
#define DEV_NAME 128

typedef struct proc_info {
  int total;
  int running;
  float load_avg_for_1;
  float load_avg_for_5;
} proc_info_t;

typedef struct mem_info {
  unsigned long long total;
  unsigned long long available;
  unsigned long long active;
  unsigned long long inactive;
  unsigned long long swapped;
  unsigned long long mapped;
} mem_info_t;

typedef struct disk_info {
  char mount[DEV_NAME];
  unsigned long long total;
  unsigned long long used;
} disk_info_t;

// typedef struct net_info {

// } net_info_t;

typedef struct engine_info {
  int motor_1_speed;
  int motor_2_speed;
} engine_info_t;

typedef struct system_info {
  char name[SYS_NAME];
  int cores;
  engine_info_t engine;
  proc_info_t process;
  mem_info_t memory;
  disk_info_t disk;
  // net_info_t network;
} system_info_t;

#endif