#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <input.h>

#define SHM_KEY_SENSOR 0x1000
#define TOY_TOK_BUFSIZE 64
#define TOY_TOK_DELIM " \t\r\n\a"
#define TOY_BUFFSIZE 1024
#define DUMP_STATE 2

typedef struct _sig_ucontext {
  unsigned long uc_flags;
  struct ucontext *uc_link;
  stack_t uc_stack;
  // struct sigcontext uc_mcontext;
  sigset_t uc_sigmask;
} sig_ucontext_t;

static pthread_mutex_t global_message_mutex = PTHREAD_MUTEX_INITIALIZER;
static char global_message[TOY_BUFFSIZE];

static mqd_t watchdog_queue, monitor_queue, disk_queue, camera_queue;
static sensor_data_t *sensor_info = NULL;

static void segfault_handler(int sig_num, siginfo_t * info, void * ucontext) {
  void *array[50];
  void *caller_address;
  char **messages;
  int size, i;
  sig_ucontext_t *uc;

  uc = (sig_ucontext_t *)ucontext;
  // caller_address = (void *) uc->uc_mcontext.rip;

  fprintf(stderr, "\n");

  if (sig_num == SIGSEGV)
    printf("signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), info->si_addr, (void *) caller_address);
  else
    printf("signal %d (%s)\n", sig_num, strsignal(sig_num));

  size = backtrace(array, 50);
  array[1] = caller_address;
  messages = backtrace_symbols(array, size);

  for (i = 1; i < size && messages != NULL; ++i) {
    printf("[bt]: (%d) %s\n", i, messages[i]);
  }

  free(messages);
  exit(EXIT_FAILURE);
}

char *toy_cmd_str[] = {
  "send",
  "mu",
  "sh",
  "mq",
  "elf",
  "dump",
  "mincore",
  "busy",
  "exit"
};

int (*toy_cmd_func[]) (char **) = {
  &toy_send,
  &toy_mutex,
  &toy_shell,
  &toy_message_queue,
  &toy_read_elf_header,
  &toy_dump_state,
  &toy_mincore,
  &toy_busy,
  &toy_exit
};

int toy_num_builtins() {
  return sizeof(toy_cmd_str) / sizeof(char *);
}

int toy_send(char **args) {
  printf("send message: %s\n", args[1]);

  return 1;
}

int toy_mutex(char **args) {
  if (args[1] == NULL) return 1;

  printf("save message: %s\n", args[1]);
  pthread_mutex_lock(&global_message_mutex);
  strcpy(global_message, args[1]);
  pthread_mutex_unlock(&global_message_mutex);
  return 1;
}

int toy_message_queue(char **args) {
  int mqretcode;
  toy_msg_t msg;

  if (args[1] == NULL || args[2] == NULL) return 1;

  if (!strcmp(args[1], "camera")) {
    msg.msg_type = atoi(args[2]);
    msg.param1 = 0;
    msg.param2 = 0;
    mqretcode = mq_send(camera_queue, (char *)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);
  }

  return 1;
}

int toy_read_elf_header(char **args) {
  int mqretcode;
  toy_msg_t msg;
  int in_fd;
  char *contents = NULL;
  size_t contents_sz;
  struct stat st;
  Elf64Hdr *map;

  in_fd = open("./sample/sample.elf", O_RDONLY);
  if (in_fd < 0) {
    printf("cannot open ./sample/sample.elf\n");
    return 1;
  }

  if (!fstat(in_fd, &st)) {
    contents_sz = st.st_size;
    if (!contents_sz) {
      printf("./sample/sample.elf is empty\n");
      return 1;
    }
    printf("real size: %ld\n", contents_sz);
    map = (Elf64Hdr *)mmap(NULL, contents_sz, PROT_READ, MAP_PRIVATE, in_fd, 0);
    printf("Object file type : %d\n", map->e_type);
    printf("Architecture : %d\n", map->e_machine);
    printf("Object file version : %d\n", map->e_version);
    printf("Entry point virtual address : %ld\n", map->e_entry);
    printf("Program header table file offset : %ld\n", map->e_phoff);
    munmap(map, contents_sz);
  }

  return 1;
}

int toy_dump_state(char **args) {
  int mqretcode;
  toy_msg_t msg;

  msg.msg_type = DUMP_STATE;
  msg.param1 = 0;
  msg.param2 = 0;
  mqretcode = mq_send(camera_queue, (char *)&msg, sizeof(msg), 0);
  assert(mqretcode == 0);
  mqretcode = mq_send(monitor_queue, (char *)&msg, sizeof(msg), 0);
  assert(mqretcode == 0);

  return 1;
}

int toy_mincore(char **args) {
  unsigned char vec[20];
  int res;
  size_t page = sysconf(_SC_PAGESIZE);
  void *addr = mmap(NULL, 20 * page, PROT_READ | PROT_WRITE, MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  res = mincore(addr, 10 * page, vec);
  assert(res == 0);

  return 1;
}

int toy_busy(char **args) {
  while (1);
  return 1;
}

int toy_exit(char **args) {
  return 0;
}

int toy_shell(char **args) {
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("toy");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("toy");
  } else {
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int toy_execute(char **args) {
  if (args[0] == NULL) return 1;

  for (int i = 0; i < toy_num_builtins(); i++) {
    if (strcmp(args[0], toy_cmd_str[i]) == 0) {
      return (*toy_cmd_func[i])(args);
    }
  }

  return 1;
}

char *toy_read_line(void) {
  char *line = NULL;
  ssize_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else {
      perror(": getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
}

char **toy_split_line(char *line) {
  int bufsize = TOY_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "toy: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOY_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += TOY_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char *));

      if (!tokens) {
        free(tokens_backup);
        fprintf(stderr, "toy: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOY_TOK_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

void toy_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf("TOY> ");
    line = toy_read_line();
    args = toy_split_line(line);
    status = toy_execute(args);
    free(line);
    free(args);
  } while (status);
}


void *sensor_thread(void* arg) {
  int mqretcode;
  char *s = arg;
  toy_msg_t msg;

  printf("%s", s);

  while (1) {
      posix_sleep_ms(10000);
    if (sensor_info != NULL) {
      sensor_info ->temperature = 35;
      sensor_info ->pressure = 55;
      sensor_info ->humidity = 80;
    }
    msg.msg_type = 1;
    msg.param1 = SHM_KEY_SENSOR;
    msg.param2 = 0;
    mqretcode = mq_send(monitor_queue, (char *)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);
  }

  return 0;
}

void *command_thread(void* arg) {
  char *s = arg;

  printf("%s", s);
  toy_loop();

  return 0;
}

int input() {
  int retcode;
  struct sigaction sa;
  pthread_t command_thread_tid, sensor_thread_tid;
  int i;
  scmp_filter_ctx ctx;

  memset(&sa, 0, sizeof(sigaction));
  sigemptyset(&sa.sa_mask);

  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sa.sa_sigaction = segfault_handler;
  sigaction(SIGSEGV, &sa, NULL);

  ctx = seccomp_init(SCMP_ACT_ALLOW);
  if (ctx == NULL) {
    printf("seccomp_init failed");
    return -1;
  }

  int rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mincore), 0);
  if (rc < 0) {
    printf("seccomp_rule_add failed");
    return -1;
  }

  seccomp_export_pfc(ctx, 5);
  seccomp_export_bpf(ctx, 6);

  rc = seccomp_load(ctx);
  if (rc < 0) {
    printf("seccomp_load failed");
    return -1;
  }
  seccomp_release(ctx);

  sensor_info = (sensor_data_t *)shm_create(SHM_KEY_SENSOR, sizeof(sensor_data_t));
  if (sensor_info == (void *)-1) {
    sensor_info = NULL;
    printf("Error in shm_create SHMID=%d SHM_KEY_SENSOR\n", SHM_KEY_SENSOR);
  }

  watchdog_queue = mq_open("/watchdog_queue", O_RDWR);
  assert(watchdog_queue != -1);
  monitor_queue = mq_open("/monitor_queue", O_RDWR);
  assert(monitor_queue != -1);
  disk_queue = mq_open("/disk_queue", O_RDWR);
  assert(disk_queue != -1);
  camera_queue = mq_open("/camera_queue", O_RDWR);
  assert(camera_queue != -1);

  retcode = pthread_create(&command_thread_tid, NULL, command_thread, "command thread\n");
  assert(retcode == 0);
  retcode = pthread_create(&sensor_thread_tid, NULL, sensor_thread, "sensor thread\n");
  assert(retcode == 0);

  while (1) {
    sleep(1);
  }

  return 0;
}

pid_t create_input() {
  pid_t input_pid;
  const char *name = "input";
  
  printf("입력 프로세스를 생성합니다.\n");

  switch (input_pid = fork()) {
  case -1:
    printf("input fork failed\n");
  case 0:
    if (prctl(PR_SET_NAME, (unsigned long)name) < 0)
        perror("prctl()");
    input();
    break;
  default:
    break;
  }

  return input_pid;
}
