#ifndef _INPUT_PROCESS_H_
#define _INPUT_PROCESS_H_

#include <stdint.h>

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

pid_t create_input_process();
void init_input_process();

void *command_thread(void *);
void *sensor_thread(void *);
void *mosq_thread(void *args);

int command_busy(char **args);
int command_dump(char **args);
int command_elf(char **args);
int command_exit(char **args);
int command_set_motor_1_speed(char **argv);
int command_set_motor_2_speed(char **argv);
int command_mincore(char **args);
int command_mu(char **args);
int command_mq(char **args);
int command_sh(char **args);
int command_simple_io(char **argv);

#endif