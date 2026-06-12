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
    "syscall\n\t"
    "mov %%eax, %0\n\t"
    : "=a" (res)
    : "a" ((long)SYS_WRITE),
      "D" ((long)fd),
      "S" (buf),
      "d" (count)
    : "rcx", "r11", "memory", "cc"
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

void printf32(float x)
{
  char buf[50];
  if (x == 0)
  {
    write(1, "0.0\n", 4);
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
  write(1, buf, i);
}

void printf64(double x)
{
  char buf[50];
  if (x == 0)
  {
    write(1, "0.0\n", 4);
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
  write(1, buf, i);
}

void printstr(char* x)
{
  write(1, x, strlen(x));
}

void printchar(char x)
{
  write(1, &x, 1);
}

void print(const char* buf)
{ write(1, buf, strlen(buf)); }
