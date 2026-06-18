#include "string.h"
#include "sys_wrapper.h"

/*
long write(int fd, const char* buf, long count)
{
  long res;
  asm volatile (
    "syscall\n\t"
    : "=a" (res)
    : "a" ((long)SYS_WRITE),
      "D" ((long)fd),
      "S" (buf),
      "d" (count)
    : "rcx", "r11", "memory", "cc"
  );
  return res;
}

long read(int fd, char* buf, long count)
{
  long res;
  asm volatile (
    "syscall\n\t"
    : "=a" (res)
    : "a" ((long)SYS_READ),
      "D" ((long)fd),
      "S" (buf),
      "d" (count)
    : "rcx", "r11", "memory", "cc"
  );
  return res;
}

long open(const char* filename, int flags, int mode)
{
  long res;
  asm volatile (
    "syscall\n\t"
    : "=a" (res)
    : "a" ((long)SYS_OPEN),
      "D" (filename),
      "S" ((long)flags),
      "d" ((long)mode)
    : "rcx", "r11", "memory", "cc"
  );
  return res;
}

long close(int fd)
{
  long res;
  asm volatile (
    "syscall\n\t"
    : "=a" (res)
    : "a" ((long)SYS_CLOSE),
      "D" ((long)fd)
    : "rcx", "r11", "memory", "cc"
  );
  return res;
}
*/

void printi32(int32_t x)
{
  char buf[13];
  if (x == 0)
  {
    syscall(SYS_WRITE, 1, "0\n", 2);
    // write(1, "0\n", 2);
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
  syscall(SYS_WRITE, 1, buf, i);
  // write(1, buf, i);
}

void printi64(int64_t x)
{
  char buf[23];
  if (x == 0)
  {
    syscall(SYS_WRITE, 1, "0\n", 2);
    // write(1, "0\n", 2);
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
  syscall(SYS_WRITE, 1, buf, i);
  // write(1, buf, i);
}

void printf32(float x)
{
  char buf[50];
  if (x == 0)
  {
    syscall(SYS_WRITE, 1, "0.0\n", 4);
    // write(1, "0.0\n", 4);
    return;
  }

  int i = 0;
  if (x < 0) buf[i++] = '-';

  float val = (x < 0 ? -(float)x : (float)x);

  uint64_t ival = (uint64_t)val;
  int j = 0;
  char temp[50];
  for (; ival > 0; ival /= 10)
    temp[j++] = (ival % 10) + '0';

  for (j--; j >= 0; j--)
    buf[i++] = temp[j];

  if (i == 0) buf[i++] = '0';
  buf[i++] = '.';

  float fval = val - (uint64_t)val;
  for (int k = 0; k < 6; k++)
  {
    fval *= 10;
    int d = ((int)fval % 10);
    buf[i++] = d + '0';
    fval -= d;
  }

  if (buf[i - 1] == '.') buf[i++] = '0';

  buf[i++] = '\n';
  buf[i] = '\0';
  syscall(SYS_WRITE, 1, buf, i);
  // write(1, buf, i);
}

void printf64(double x)
{
  char buf[50];
  if (x == 0)
  {
    syscall(SYS_WRITE, 1, "0.0\n", 4);
    // write(1, "0.0\n", 4);
    return;
  }

  int i = 0;
  if (x < 0) buf[i++] = '-';

  double val = (x < 0 ? -(double)x : (double)x);

  uint64_t ival = (uint64_t)val;
  int j = 0;
  char temp[50];
  for (; ival > 0; ival /= 10)
    temp[j++] = (ival % 10) + '0';

  for (j--; j >= 0; j--)
    buf[i++] = temp[j];

  if (i == 0) buf[i++] = '0';
  buf[i++] = '.';

  double fval = val - (uint64_t)val;
  for (int k = 0; k < 6; k++)
  {
    fval *= 10;
    int d = ((int)fval % 10);
    buf[i++] = d + '0';
    fval -= d;
  }

  if (buf[i - 1] == '.') buf[i++] = '0';

  buf[i++] = '\n';
  buf[i] = '\0';
  syscall(SYS_WRITE, 1, buf, i);
  // write(1, buf, i);
}

void printstr(char* x)
{
  syscall(SYS_WRITE, 1, x, strlen(x));
  // write(1, x, strlen(x));
}

void printchar(char x)
{
  syscall(SYS_WRITE, 1, &x, 1);
  // write(1, &x, 1);
}

void print(const char* buf)
{ syscall(SYS_WRITE, 1, buf, strlen(buf)); } // write(1, buf, strlen(buf)); }

