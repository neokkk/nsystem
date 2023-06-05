#ifndef _INPUT_H
#define _INPUT_H

#include <assert.h>
#include <errno.h>
#include <execinfo.h>
#include <mqueue.h>
#include <pthread.h>
#include <seccomp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#include <wait.h>

#include <sensor_data.h>
#include <shared_memory.h>
#include <toy_message.h>
#include <timer.h>

typedef struct {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf64Hdr;

pid_t create_input();
int input();

static void segfault_handler(int sig_num, siginfo_t* info, void* ucontext);
int toy_num_builtins();
int toy_send(char **args);
int toy_mutex(char **args);
int toy_message_queue(char **args);
int toy_read_elf_header(char **args);
int toy_dump_state(char **args);
int toy_mincore(char **args);
int toy_busy(char **args);
int toy_exit(char **args);
int toy_shell(char **args);
int toy_execute(char **args);
char *toy_read_line(void);
char **toy_split_line(char *line);
void toy_loop();
void *sensor_thread(void *arg);
void *command_thread(void *arg);

#endif /* _INPUT_H */
