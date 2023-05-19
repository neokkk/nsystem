#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#define INPUT_THREAD_NUM 2
#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"
#define BUFSIZE 1024

#include <input.h>

typedef struct _sig_ucontext {
  unsigned long uc_flags;
  struct ucontext *uc_link;
  stack_t uc_stack;
  // struct sigcontext uc_mcontext;
  sigset_t uc_sigmask;
} sig_ucontext_t;

typedef void* scmp_filter_ctx;

static pthread_mutex_t global_message_mutex = PTHREAD_MUTEX_INITIALIZER;
static sensor_data_t* sensor_data;
static mqd_t mqs[QUEUE_NUM];
static char* mq_names[QUEUE_NUM] = {"/watchdog_queue", "/monitor_queue", "/disk_queue", "/camera_queue"};
static char global_message[BUFSIZE];

static void sigsegv_handler(int sig_num, siginfo_t* info, void* ucontext)
{
  void* array[50];
  void* caller_address;
  char** messages;
  int size, i;
  sig_ucontext_t* uc;

  uc = (sig_ucontext_t*)ucontext;

  /* Get the address at the time the signal was raised */
  // caller_address = (void*) uc ->uc_mcontext.x86_64;  // RIP: x86_64 specific     arm_pc: ARM

  fprintf(stderr, "\n");

  if (sig_num == SIGSEGV)
    printf("signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), info->si_addr,
           (void*) caller_address);
  else
    printf("signal %d (%s)\n", sig_num, strsignal(sig_num));

  size = backtrace(array, 50);
  /* overwrite sigaction with caller's address */
  // array[1] = caller_address;
  // messages = backtrace_symbols(array, size);

  /* skip first stack frame (points here) */
  // for (i = 1; i < size && messages != NULL; ++i) {
  //   printf("[bt]: (%d) %s\n", i, messages[i]);
  // }

  // free(messages);
  exit(-1);
}

char* builtin_str[] = {
  "send",
  "mu",
  "sh",
  "mq",
  "exit",
  "elf",
  "mincore",
};

int (*builtin_func[])(char**) = {
  &toy_send,
  &toy_mutex,
  &toy_shell,
  &toy_message_queue,
  &toy_exit,
  &toy_read_elf_header,
  &toy_mincore,
};

int toy_num_builtins()
{
  return sizeof(builtin_str) / sizeof(char*);
}

int toy_send(char** args)
{
  printf("send message: %s\n", args[1]);
  return 0;
}

int toy_mutex(char** args)
{
  if (args[1] == NULL) {
    return -1;
  }

  printf("save message: %s\n", args[1]);

  if (pthread_mutex_lock(&global_message_mutex) != 0) {
    perror("pthread_mutex_lock error");
    return -1;
  }

  strcpy(global_message, args[1]);
  
  if (pthread_mutex_unlock(&global_message_mutex)) {
    perror("pthread_mutex_unlock error");
    return -1;
  }
  
  return 0;
}

int toy_exit(char** args)
{
  exit(0);
}

int toy_shell(char** args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) < 0) {
      perror("execvp error");
    }
    exit(-1);
  } else if (pid < 0) {
    perror("fork error");
  } else {
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 0;
}

int toy_message_queue(char** args)
{
  int mqretcode;
  toy_msg_t msg;

  if (args[1] == NULL || args[2] == NULL) {
    return 1;
  }

  if (!strcmp(args[1], "camera")) {
    msg.msg_type = atoi(args[2]);
    msg.param1 = 0;
    msg.param2 = 0;
    mqretcode = mq_send(mqs[3], (void*)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);
  }

  return 0;
}

int toy_execute(char** args)
{
  int i;

  if (args[0] == NULL) {
    return -1;
  }

  for (i = 0; i < toy_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return 0;
}

int toy_read_elf_header(char** args)
{
  int mqretcode;
  toy_msg_t msg;
  int elf_fd;
  size_t content_size;
  struct stat st;
  Elf64Hdr* map;

  if ((elf_fd = open("./sample/sample.elf", O_RDONLY)) < 0) {
    printf("cannot open ./sample/sample.elf\n");
    return -1;
  }
  
  if (fstat(elf_fd, &st) < 0) {
    perror("cannot stat ./sample/sample.elf\n");
    return -1;
  }

  map = (Elf64Hdr*)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, elf_fd, 0);
  printf("real size: %ld\n", st.st_size);
  printf("architecture: %ld\n", map ->e_machine);
  printf("file type: %ld\n", map ->e_type);
  printf("file version: %ld\n", map ->e_version);
  printf("entry point of VA: %ld\n", map ->e_entry);
  printf("elf header size: %ld\n", map ->e_type);

  return 1;
}

int toy_mincore(char** args)
{
  unsigned char vec[20];
  int res;
  size_t page = sysconf(_SC_PAGESIZE);
  void* addr = mmap(NULL, 20 * page, PROT_READ | PROT_WRITE, MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  res = mincore(addr, 20 * page, vec);
  assert(res == 0);

  return 1;
}

char* toy_read_line(void)
{
  char* line = NULL;
  ssize_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else {
      perror(": getline\n");
      exit(-1);
    }
  }
  return line;
}

char** toy_split_line(char* line)
{
  int bufsize = TOK_BUFSIZE, position = 0;
  char** tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "toy: allocation error\n");
    exit(-1);
  }

  token = strtok(line, TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));

      if (!tokens) {
        free(tokens_backup);
        fprintf(stderr, "toy: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOK_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

void toy_loop()
{
  char* line;
  char** args;
  int status;

  do {
    printf("TOY> ");
    line = toy_read_line();
    args = toy_split_line(line);
    status = toy_execute(args);
    printf("status: %d\n", status);

    free(line);
    free(args);
  } while (1);
}

int get_shm_id(key_t shm_key, int size) {
  int shmid;
  if ((shmid = shmget(shm_key, size, 0666)) < 0) {
    perror("shmget error");
    printf("errno: %d\n", errno);
    exit(-1);
  }
  printf("shmid: %d\n", shmid);
  return shmid;
}

void* command_thread(void* arg)
{
  printf("command thread is running\n");
  toy_loop();
  exit(EXIT_SUCCESS);
}

void* sensor_thread(void* arg)
{
  int thread_id = (int)arg;
  toy_msg_t msg;
  int semid, shmid;
  int mqretcode;

  printf("sensor thread is running\n");

  shmid = get_shm_id(SHM_SENSOR_KEY, sizeof(sensor_data_t));

  while (1) {
    posix_sleep_ms(5000);

    if (sensor_data == NULL) continue;

    sensor_data ->humidity = rand() % 40;
    sensor_data ->temperature = rand() % 20 + 20;
    sensor_data ->pressure = 30;

    msg.msg_type = 1;
    msg.param1 = shmid;
    msg.param2 = 0;
    mqretcode = mq_send(mqs[1], (char *)&msg, sizeof(msg), 0);
    printf("sensor data send \n");
    assert(mqretcode == 0);
  }
}

int input()
{
  pthread_t threads[INPUT_THREAD_NUM];
  void* (*thread_funcs[INPUT_THREAD_NUM])(void*) = {command_thread, sensor_thread};
  struct sigaction sa;
  scmp_filter_ctx ctx;

  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sa.sa_sigaction = sigsegv_handler;
  sigemptyset(&sa.sa_mask);

  ctx = seccomp_init(SCMP_ACT_ALLOW);
  if (ctx == NULL) {
    perror("seccomp_init error\n");
    return -1;
  }

  if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mincore), 0) < 0) {
    perror("seccomp_rule_add error");
    return -1;
  }

  if (seccomp_load(ctx) < 0) {
    seccomp_release(ctx);
    perror("seccomp_load error");
    return -1;
  }

  seccomp_release(ctx);

  if (sigaction(SIGSEGV, &sa, NULL) < 0) { // SIGSEVG 시그널 핸들러 등록
    perror("sigaction error");
    return -1;
  }

  sensor_data = (sensor_data_t*)shm_create(SHM_SENSOR_KEY, sizeof(sensor_data_t)); // 센서 데이터 공유 메모리 생성
  if (sensor_data < (void*)0) { // 센서 데이터 공유 메모리 생성
    perror("shm_create error");
    return -1;
  }

  for (int i = 0; i < QUEUE_NUM; i++) { // 메시지 큐 열기
    if ((mqs[i] = mq_open(mq_names[i], O_RDONLY)) < 0) {
      perror("mq_open error");
      return -1;
    }
  }

  for (int i = 0; i < INPUT_THREAD_NUM; i++) {
    if (pthread_create(&threads[i], NULL, thread_funcs[i], i) != 0) {
      perror("pthread_create error\n");
      return -1;
    }
  }

  printf("<== system\n");

  while (1) {
    sleep(1);
  }

  return 0;
}

pid_t create_input()
{
  pid_t input_pid;
  const char *process_name = "input";

  printf("입력 프로세스를 생성합니다.\n");

  switch (input_pid = fork()) {
    case -1:
      perror("input fork error");
      exit(-1);
    case 0:
      if (prctl(PR_SET_NAME, process_name) < 0) {
        perror("prctl error");
        exit(-1);
      }
      input();
      break;
    default:
      printf("input pid: %d\n", input_pid);
  }

  return input_pid;
}
