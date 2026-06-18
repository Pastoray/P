#ifndef SIGHANDLE_H
#define SIGHANDLE_H

#define SIGINT 2
#define SIGFPE 8
#define SIGSEGV 11

#define SA_SIGINFO 0x00000004
#define SA_RESTORER 0x04000000

typedef struct siginfo
{
  int si_signo;
  int si_errno;
  int si_code;
} siginfo_t;

typedef struct mcontext
{
  unsigned long long gregs[23];
} mcontext_t;

typedef struct ucontext
{
  void* uc_flags;
  void* uc_link;
  void* uc_stack;
  struct mcontext uc_mcontext;
} ucontext_t;

typedef struct kernel_sigact
{
  void (*handler)(int, siginfo_t*, void*);
  unsigned long flags;
  void (*restorer)();
  unsigned long mask;
} kernel_sigact_t;

void signal_restorer();
int set_sigact(int, void (*)(int, siginfo_t*, void*));
void handle_sigint(int, siginfo_t*, void*);
void handle_sigsegv(int, siginfo_t*, void*);

#endif // SIGHANDLE_H
