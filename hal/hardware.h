#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdint.h>

#define HAL_MODULE_INFO_SYM HMI
#define HAL_MODULE_INFO_SYM_AS_STR "HMI"

#define MAKE_TAG_CONSTANT(A, B, C, D) (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))

#define HARDWARE_MODULE_TAG MAKE_TAG_CONSTANT('H', 'W', 'M', 'T')
#define HARDWARE_DEVICE_TAG MAKE_TAG_CONSTANT('H', 'W', 'D', 'T')

#define HARDWARE_MODULE_ID  "oem_camera"

typedef struct camera_data {
    void *data;
    size_t size;
    void *handle;
} camera_data_t;

typedef void (*camera_notify_callback)(int32_t msg_type, int32_t ext1, int32_t ext2);
typedef void (*camera_data_callback)(int32_t msg_type, const camera_data_t *data, unsigned int index);

typedef struct hw_module {
    uint32_t tag;
    const char *id;
    const char *name;
    int (*open)();
    int (*take_picture)();
    int (*dump)();
    int (*set_callbacks)(camera_notify_callback notify_cb, camera_data_callback data_cb);
    void* dso;
} hw_module_t;

int get_camera_module(const hw_module_t **module);

#endif