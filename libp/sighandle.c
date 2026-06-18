#include "defs.h"
#include "sys_wrapper.h"
#include "sighandle.h"
#include "print.h"

int set_sigact(int signum, void (*fn)(int, siginfo_t*, void*))
{
  struct kernel_sigact sa;
  sa.handler = fn;
  sa.flags = SA_SIGINFO | SA_RESTORER;
  sa.restorer = signal_restorer;
  sa.mask = 0;

  return syscall(SYS_RT_SIGACTION, signum, &sa, NULL, 8);
}

void handle_sigint(int signum, siginfo_t* info, void* ucontext)
{
  (void)signum;
  (void)info;
  (void)ucontext;

  printstr("\n[SIGNAL INTERCEPTED] SIGINT Detect, Shutting down..\n");
  syscall(SYS_EXIT, 0);
}

void handle_sigsegv(int signum, siginfo_t* info, void* ucontext)
{
  (void)signum;
  (void)info;
  (void)ucontext;

  printstr("\n[SIGNAL INTERCEPTED] SIGSEGV Detect, Shutting down..\n");
  syscall(SYS_EXIT, -11);
}

