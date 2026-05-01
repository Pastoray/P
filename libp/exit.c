#include "defs.h"

void exit(int32_t status)
{
  asm volatile (
    "movl %0, %%eax\n\t"
    "movl %1, %%edi\n\t"
    "syscall\n\t"
    :
    : "r" ((int)SYS_EXIT), "r" (status)
    : "rax", "edi", "memory"
  );
  __builtin_unreachable();
}