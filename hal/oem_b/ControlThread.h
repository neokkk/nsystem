#ifndef _CONTROL_THREAD_H_
#define _CONTROL_THREAD_H_

class ControlThread {
public:
	ControlThread();
	virtual ~ControlThread();	

	int takePicture();
	int dump();
};

#endif