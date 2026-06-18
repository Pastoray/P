#include "string.h"

long strlen(const char* buf)
{
  const char* cur = buf;
  while (*cur != '\0')
  {
    cur++;
  }
  return cur - buf;
}

int strcmp(const char* s1, const char* s2)
{
  for (int i = 0; s1[i] != '\0' && s2[i] != '\0'; i++)
  {
    if (s1[i] == '\0' && s2[i] == '\0') { return 0; }
    int diff = s1[i] - s2[i];
    if (diff != 0) return diff;
  }
  return 0;
}

