#include "defs.h"

long strlen(const char* buf)
{
  const char* cur = buf;
  while (*cur != '\0')
  {
    cur++;
  }
  return cur - buf;
}

long write(int fd, const char* buf, long count)
{
  int res;
  asm volatile (
    "mov %1, %%rax\n\t"
    "mov %2, %%rdi\n\t"
    "mov %3, %%rsi\n\t"
    "mov %4, %%rdx\n\t"
    "syscall\n\t"
    "mov %%eax, %0\n\t"
    : "=r" (res)
    : "r" ((long)SYS_WRITE),
      "r" ((long)fd),
      "r" (buf),
      "r" (count)
    : "rax", "rdi", "rsi", "rdx", "memory",
      "rcx", "r11"
  );
  return res;
}

void printi32(int32_t x)
{
  char buf[13];
  if (x == 0)
  {
    write(1, "0\n", 2);
    return;
  }

  int i = 0;
  if (x < 0) buf[i++] = '-';

  uint32_t val = (x < 0 ? -(uint32_t)x : (uint32_t)x);

  int j = 0;
  char temp[13];
  for (; val > 0; val /= 10)
    temp[j++] = (val % 10) + '0';

  for (j--; j >= 0; j--)
    buf[i++] = temp[j];

  buf[i++] = '\n';
  buf[i] = '\0';
  write(1, buf, i);
}

void printi64(int64_t x)
{
  char buf[23];
  if (x == 0)
  {
    write(1, "0\n", 2);
    return;
  }

  int i = 0;
  if (x < 0) buf[i++] = '-';

  uint64_t val = (x < 0 ? -(uint64_t)x : (uint64_t)x);

  int j = 0;
  char temp[23];
  for (; val > 0; val /= 10)
    temp[j++] = (val % 10) + '0';

  for (j--; j >= 0; j--)
    buf[i++] = temp[j];

  buf[i++] = '\n';
  buf[i] = '\0';
  write(1, buf, i);
}

void print(const char* buf)
{ write(1, buf, strlen(buf)); }
