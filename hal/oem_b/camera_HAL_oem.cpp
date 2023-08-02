#include <cstddef>
#include <errno.h>
#include <iostream>

#include <hardware.h>

#include "camera_HAL_oem.h"
#include "ControlThread.h"

static ControlThread *control_thread;

using std::cout;
using std::endl;

int oem_camera_open(void)
{
	cout << "[OEM-B] open camera" << endl;

	control_thread = new ControlThread();
	if (!control_thread) {
		cout << "[OEM-B] fail to allocate memory" << endl;
		return -ENOMEM;
	}

	return 0;
}

int oem_camera_take_picture(void)
{
	return control_thread->takePicture();
}

int oem_camera_dump(void)
{
	return control_thread->dump();
}

hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.id = HARDWARE_MODULE_ID,
	.name = "OEM-B Camera Hardware Module",
	.open = oem_camera_open,
	.take_picture = oem_camera_take_picture,
	.dump = oem_camera_dump,
};