#ifndef _CAMERA_HAL_OEM_H_
#define _CAMERA_HAL_OEM_H_

#ifdef __cplusplus
extern "C" {
#endif

int oem_camera_open(void);
int oem_camera_take_picture(void);
int oem_camera_dump(void);

#ifdef __cplusplus
}
#endif

#endif