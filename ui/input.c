#include <input.h>

typedef struct _sig_ucontext {
  unsigned long uc_flags;
  struct ucontext *uc_link;
  stack_t uc_stack;
  struct sigcontext uc_mcontext;
  sigset_t uc_sigmask;
} sig_ucontext_t;

void sigsegv_handler(int sig_num, siginfo_t* info, void* ucontext)
{
  void* array[50];
  void* caller_address;
  char** messages;
  int size, i;
  sig_ucontext_t* uc;

  uc = (sig_ucontext_t*)ucontext;

  /* Get the address at the time the signal was raised */
  caller_address = (void *) uc ->uc_mcontext.rip;  // RIP: x86_64 specific     arm_pc: ARM

  fprintf(stderr, "\n");

  if (sig_num == SIGSEGV)
    printf("signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), info->si_addr,
           (void *) caller_address);
  else
    printf("signal %d (%s)\n", sig_num, strsignal(sig_num));

  size = backtrace(array, 50);
  /* overwrite sigaction with caller's address */
  array[1] = caller_address;
  messages = backtrace_symbols(array, size);

  /* skip first stack frame (points here) */
  for (i = 1; i < size && messages != NULL; ++i) {
    printf("[bt]: (%d) %s\n", i, messages[i]);
  }

  free(messages);
  exit(EXIT_FAILURE);
}

int input()
{
  printf("input process is running!\n");

  if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
    perror("signal error");
    exit(-1);
  }

  while (1) {
    sleep(1);
  }

  exit(0);
}

pid_t create_input()
{
  pid_t input_pid;
  const char *process_name = "input";

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
