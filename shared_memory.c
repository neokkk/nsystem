#include <shared_memory.h>

void* shm_create(int key, int size) {
  int shmid;
  void* shmaddr;

  if ((shmid = shmget((key_t)key, size, IPC_CREAT | IPC_EXCL | 0666)) < 0) {
    if (errno == EEXIST) {
      shmid = shmget(key, size, 0666);
    } else {
      perror("shmget error");
      return (void*)-1;
    }
  }

  if ((shmaddr = shm_attach(shmid)) < (void*)0) {
    perror("shmat error");
    return (void*)-1;
  }

  return shmaddr;
}

void* shm_attach(int shmid) {
  void* shmaddr;

  if ((shmaddr = shmat(shmid, NULL, 0)) == (void*)-1) {
    perror("shmat error");
    return (void*)-1;
  }

  return shmaddr;
}

int shm_detach(void* shmaddr) {
  if (shmdt(shmaddr) < 0) {
    perror("shmdt error");
    return -1;
  }

  return 0;
}

int shm_delete(int shmid) {
  if (shmctl(shmid, IPC_RMID, 0) < 0) {
    perror("shmctl(RMID) error");
    return -1;
  }

  return 0;
}
