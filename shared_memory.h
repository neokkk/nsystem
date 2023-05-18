#ifndef _SHARED_MEMORY_H
#define _SHARED_MEMORY_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>

void* shm_create(int key, int size);
void* shm_attach(int shmid);
int shm_detach(void* shamaddr);
int shm_delete(int shmid);
int shm_get_keyid(int key);

#endif /* _SHARED_MEMORY_H */
