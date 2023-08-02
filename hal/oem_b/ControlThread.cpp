#include <iostream>

#include "ControlThread.h"

using std::cout;
using std::endl;

ControlThread::ControlThread() {
	cout << "[OEM-B] constructed ControlThread" << endl;
}

ControlThread::~ControlThread() {
	cout << "[OEM-B] destructed ControlThread" << endl;
}

int ControlThread::takePicture() {
	cout << "[OEM-B] take picture" << endl;
	return 0;
}

int ControlThread::dump() {
	cout << "[OEM-B] dump" << endl;
	return 0;
}