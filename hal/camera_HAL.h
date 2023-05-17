#ifndef _CAMERA_HAL_H_
#define _CAMERA_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <cstdio>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

int toy_camera_open(void);
int toy_camera_take_picture(void);

#ifdef __cplusplus
} // extern "C"
#endif  // __cplusplus

#endif /* _CAMERA_HAL_H_ */
