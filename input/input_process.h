#ifndef _INPUT_PROCESS_H_
#define _INPUT_PROCESS_H_

pid_t create_input_process();
void init_input_process();

void *command_thread(void *);
void *sensor_thread(void *);

int command_busy(char **args);
int command_dump(char **args);
int command_elf(char **args);
int command_exit(char **args);
int command_gpio(char **args);
int command_mincore(char **args);
int command_mu(char **args);
int command_mq(char **args);
int command_send(char **args);
int command_sh(char **args);

#endif