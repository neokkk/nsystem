#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>

#include <hardware.h>

#define HAL_LIB_CAMERA "./libcamera.so"

static int load(const hw_module_t **hmi_p)
{
	int status = 0;
	const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
	void *handle;
	hw_module_t *hmi;

	handle = dlopen(HAL_LIB_CAMERA, RTLD_NOW);
	if (!handle) {
		char *err = dlerror();
		printf("fail to load module\n");
		status = -EINVAL;
		hmi = NULL;
		return status;
	}

	hmi = (hw_module_t *)dlsym(handle, sym);
	if (!hmi) {
		printf("fail to find symbol %s\n", sym);
		status = -EINVAL;
		hmi = NULL;
		return status;
	}

	printf("loaded HAL hmi: %p, handle: %p\n", hmi_p, handle);
	*hmi_p = hmi;
	return status;
}

int get_camera_module(const hw_module_t **module)
{
	return load(module);
}