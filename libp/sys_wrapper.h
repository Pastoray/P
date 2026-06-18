#ifndef SYS_WRAPPER_H
#define SYS_WRAPPER_H

#include "defs.h"

struct timespec
{
  long tv_sec;
  long tv_nsec;
};

#define __SYSCALL_NARGS_X(a, b, c, d, e, f, g, h, n, ...) n
#define __SYSCALL_NARGS(...) __SYSCALL_NARGS_X(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0)
#define __SYSCALL_CONCAT_X(a, b) a##b
#define __SYSCALL_CONCAT(a, b) __SYSCALL_CONCAT_X(a, b)
#define __SYSCALL_DISP(a, ...) __SYSCALL_CONCAT(a, __SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

static inline long __syscall0_internal(long n)
{
  long ret;
  asm volatile ("syscall" : "=a"(ret) : "a"(n) : "rcx", "r11", "memory", "cc");
  return ret;
}

static inline long __syscall1_internal(long n, long a)
{
  long ret;
  asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a) : "rcx", "r11", "memory", "cc");
  return ret;
}

static inline long __syscall2_internal(long n, long a, long b)
{
  long ret;
  asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a), "S"(b) : "rcx", "r11", "memory", "cc");
  return ret;
}

static inline long __syscall3_internal(long n, long a, long b, long c)
{
  long ret;
  asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory", "cc");
  return ret;
}

static inline long __syscall4_internal(long n, long a, long b, long c, long d)
{
  long ret;
  register long r10 asm("r10") = d;
  asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10) : "rcx", "r11", "memory", "cc");
  return ret;
}

static inline long __syscall5_internal(long n, long a, long b, long c, long d, long e)
{
  long ret;
  register long r10 asm("r10") = d;
  register long r8 asm("r8")  = e;
  asm volatile (
    "syscall" 
    : "=a"(ret) 
    : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8) 
    : "rcx", "r11", "memory", "cc"
  );
  return ret;
}

static inline long __syscall6_internal(long n, long a, long b, long c, long d, long e, long f)
{
  long ret;
  register long r10 asm("r10") = d;
  register long r8 asm("r8")  = e;
  register long r9 asm("r9")  = f;
  asm volatile (
    "syscall" 
    : "=a"(ret) 
    : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9) 
    : "rcx", "r11", "memory", "cc"
  );
  return ret;
}

#define syscall(...) __SYSCALL_DISP(__syscall, __VA_ARGS__)

#define __syscall0(n) __syscall0_internal((long)(n))
#define __syscall1(n, a) __syscall1_internal((long)(n), (long)(a))
#define __syscall2(n, a, b) __syscall2_internal((long)(n), (long)(a), (long)(b))
#define __syscall3(n, a, b, c) __syscall3_internal((long)(n), (long)(a), (long)(b), (long)(c))
#define __syscall4(n, a, b, c, d) __syscall4_internal((long)(n), (long)(a), (long)(b), (long)(c), (long)(d))
#define __syscall5(n, a, b, c, d, e) __syscall5_internal((long)(n), (long)(a), (long)(b), (long)(c), (long)(d), (long)(e))
#define __syscall6(n, a, b, c, d, e, f) __syscall6_internal((long)(n), (long)(a), (long)(b), (long)(c), (long)(d), (long)(e), (long)(f))

int64_t sys_read(int fd, void* buf, size_t count);
int64_t sys_write(int fd, const void* buf, size_t count);
int64_t sys_open(const char* filename, int flags, int mode);
int64_t sys_close(int fd);
int64_t sys_brk(void* addr);
int64_t sys_clone(unsigned long flags, void* child_stack, int* parent_tid, int* child_tid, unsigned long tls);
void sys_exit(int status);
int64_t sys_futex(int* uaddr, int op, int val, const struct timespec* timeout, int *uaddr2, int val3);


#endif // SYS_WRAPPER_H
