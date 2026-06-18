#include "thread.h"
#include "defs.h"
#include "malloc.h"
#include "sys_wrapper.h"

static rt_thread_t s_thread_pool[MAX_THREADS];
static int s_thread_cnt = 0;

/*
long clone(
  unsigned long flags,
  void* child_stack,
  void (*wrapper)(void (*)(void), rt_thread_t*),
  void (*fn)(void),
  rt_thread_t* ctx
)
{
  long res;
  asm volatile (
    "subq $16, %%rsi\n\t"
    "movq %4, (%%rsi)\n\t"
    "movq %3, 8(%%rsi)\n\t"
    "syscall\n\t"

    "testq %%rax, %%rax\n\t"
    "jnz Lpar\n\t"
    "popq %%rdi\n\t"
    "popq %%rsi\n\t"
    "call *%6\n\t"
    "hlt"

    ".Lpar:\n\t"
    : "=a" (res)
    : "a" ((long)SYS_CLONE),
      "D" (flags),
      "S" (child_stack),
      "r" (fn),
      "r" (ctx),
      "r" (wrapper)
    : "rcx", "r11", "memory", "cc"
  );
  return res;
}
*/

/*
long futex(int* uaddr, int futex_op, int val, const void* timeout)
{
  long res;
  asm volatile (
    "syscall\n\t"
    : "=a" (res)
    : "a" ((long)SYS_FUTEX),
      "D" (uaddr),
      "S" ((long)futex_op),
      "d" ((long)val)
    : "rcx", "r11", "memory", "cc"
  );
  return res;
}
*/

void __thread_entry_wrapper(void (*thread_fn)(void), rt_thread_t* ctx)
{
  thread_fn();
  ctx->done = 1;

  syscall(SYS_FUTEX, (int*)&ctx->done, FUTEX_WAKE, 1, NULL);
  syscall(SYS_EXIT, 0);
  /*
  futex((int*)&ctx->done, FUTEX_WAKE, 1, NULL);
  exit(0);
  */
}

int spawn_thread(void (*thread_fn)(void))
{
  lock_heap();
  if (s_thread_cnt >= MAX_THREADS)
  {
    unlock_heap();
    return -1;
  }

  rt_thread_t* ctx = &s_thread_pool[s_thread_cnt++];
  void* stack = malloc(THREAD_STACK_SIZE);
  if (!stack)
  {
    unlock_heap();
    return -1;
  }

  void* top = (char*)stack + THREAD_STACK_SIZE;
  ctx->stack = top;
  ctx->done = 0;

  unsigned long flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD;
  long pid = thread_clone(flags, top, __thread_entry_wrapper, thread_fn, ctx);

  if (pid < 0)
  {
    unlock_heap();
    return -1;
  }

  ctx->id = pid;
  return ctx->id;
}

void join_thread(int tid)
{
  for (int i = 0; i < MAX_THREADS; i++)
  {
    rt_thread_t* ctx = &s_thread_pool[i];
    int ctid = ctx->id;
    if (ctid == tid)
    {
      while (!ctx->done) syscall(SYS_FUTEX, (int*)&ctx->done, FUTEX_WAIT, 0, NULL); // futex((int*)&ctx->done, FUTEX_WAIT, 0, NULL);
      return;
    }
  }
}

void join_all_threads(void)
{
  for (int i = 0; i < s_thread_cnt; i++)
  {
    int tid = s_thread_pool[i].id;
    join_thread(tid);
  }

  lock_heap();
  s_thread_cnt = 0;
  unlock_heap();
}

void mutex_lock(rt_mutex_t* mtx)
{
  while (__atomic_test_and_set(&mtx->state, __ATOMIC_ACQUIRE))
  {
    syscall(SYS_FUTEX, (int*)&mtx->state, FUTEX_WAIT, 1, NULL);
    // futex((int*)&mtx->state, FUTEX_WAIT, 1, NULL);
  }
}

void mutex_unlock(rt_mutex_t* mtx)
{
  __atomic_clear(&mtx->state, __ATOMIC_RELEASE);
  syscall(SYS_FUTEX, (int*)&mtx->state, FUTEX_WAKE, 1, NULL);
  // futex((int*)&mtx->state, FUTEX_WAKE, 1, NULL);
}

void thread_sleep(long secs, long nanosecs)
{
  struct timespec ts;
  ts.tv_sec = secs;
  ts.tv_nsec = nanosecs;

  syscall(SYS_NANOSLEEP, &ts, NULL);
}

int exec_prog(const char* filename, const char* argv[], const char* envp[])
{
  return syscall(SYS_EXECVE, filename, argv, envp);
}

long wait_for_child(int pid, int* status_out)
{
  return syscall(SYS_WAIT4, pid, status_out, 0, NULL);
}

