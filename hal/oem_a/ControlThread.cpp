#include <iostream>

#include "ControlThread.h"

using std::cout;
using std::endl;

ControlThread::ControlThread() {
	cout << "[OEM-A] constructed ControlThread" << endl;
}

ControlThread::~ControlThread() {
	cout << "[OEM-A] destructed ControlThread" << endl;
}

int ControlThread::takePicture() {
	cout << "[OEM-A] take picture" << endl;
	return 0;
}

int ControlThread::dump() {
	cout << "[OEM-A] dump" << endl;
	return 0;
}