#ifndef _CONTROL_THREAD_H_
#define _CONTROL_THREAD_H_

#include <cstdio>
#include <unistd.h>

class ControlThread {

public:
  ControlThread();
  ~ControlThread();

  // 사진 찍는 메소드
  int takePicture();

private:

}; // class ControlThread

#endif /* _CONTROL_THREAD_H_ */
