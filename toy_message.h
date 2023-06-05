#ifndef _TOY_MESSAGE_H_
#define _TOY_MESSAGE_H_

#define QUEUE_NUM 4

#include <unistd.h>

typedef struct toy_msg {
  unsigned int msg_type;
  unsigned int param1;
  unsigned int param2;
  void *param3;
} toy_msg_t;

#endif /* _TOY_MESSAGE_H_ */