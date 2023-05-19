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
  uint8_t     e_ident[16];         /* Magic number and other info */
  uint16_t    e_type;              /* Object file type */
  uint16_t    e_machine;           /* Architecture */
  uint32_t    e_version;           /* Object file version */
  uint64_t    e_entry;             /* Entry point virtual address */
  uint64_t    e_phoff;             /* Program header table file offset */
  uint64_t    e_shoff;             /* Section header table file offset */
  uint32_t    e_flags;             /* Processor-specific flags */
  uint16_t    e_ehsize;            /* ELF header size in bytes */
  uint16_t    e_phentsize;         /* Program header table entry size */
  uint16_t    e_phnum;             /* Program header table entry count */
  uint16_t    e_shentsize;         /* Section header table entry size */
  uint16_t    e_shnum;             /* Section header table entry count */
  uint16_t    e_shstrndx;          /* Section header string table index */
} Elf64Hdr;

pid_t create_input();
int input();

static void sigsegv_handler(int sig_num, siginfo_t* info, void* ucontext);
int toy_num_builtins();
int toy_send(char** args);
int toy_mutex(char** args);
int toy_message_queue(char** args);
int toy_read_elf_header(char **args);
int toy_mincore(char** args);
int toy_exit(char** args);
int toy_shell(char** args);
int toy_execute(char** args);
char** toy_split_line(char* line);
char* toy_read_line(void);
void toy_loop();
void* sensor_thread(void* arg);
void* command_thread(void* arg);

#endif /* _INPUT_H */
