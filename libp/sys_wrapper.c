#include "sys_wrapper.h"
#include "defs.h"
#include "print.h"
#include "sighandle.h"

int64_t sys_read(int fd, void* buf, size_t count)
{
  return syscall(SYS_READ, fd, buf, count);
}

int64_t sys_write(int fd, const void* buf, size_t count)
{
  return syscall(SYS_WRITE, fd, buf, count);
}

int64_t sys_open(const char* filename, int flags, int mode)
{
  return syscall(SYS_OPEN, (int64_t)filename, flags, mode);
}

int64_t sys_close(int fd)
{

  return syscall(SYS_CLOSE, fd);
}

int64_t sys_brk(void* addr)
{
  return syscall(SYS_BRK, (int64_t)addr);
}

int64_t sys_clone(unsigned long flags, void* child_stack, int* parent_tid, int* child_tid, unsigned long tls)
{
  return syscall(SYS_CLONE, flags, (int64_t)child_stack, (int64_t)parent_tid, (int64_t)child_tid, tls);
}

void sys_exit(int status)
{
  syscall(SYS_EXIT, status);
}

int64_t sys_futex(int* uaddr, int op, int val, const struct timespec* timeout, int* uaddr2, int val3)
{
  return syscall(SYS_FUTEX, (int64_t)uaddr, op, val, (int64_t)timeout, (int64_t)uaddr2, val3);
}

