#include "memory.h"
#include "sys_wrapper.h"
#include "defs.h"
#include "malloc.h"

void* memset(void* ptr, int c, size_t n)
{
  char v = c;
  for (int i = 0; i < n; i++)
    ((char*)ptr)[i] = v;

  return ptr;
}

void* memcpy(void* dest, const void* src, size_t n)
{
  for (int i = 0; i < n; i++)
    ((char*)dest)[i] = ((char*)src)[i];

  return dest;
}

void* memmove(void* dest, const void* src, size_t n)
{
  char* d = (char*)dest;
  char* s = (char*)src;

  if (d <= s)
    for (size_t i = 0; i < n; i++)
      d[i] = s[i];
  else
    for (size_t i = n; i > 0; i--)
      d[i - 1] = s[i - 1];

  return dest;
}

void* realloc(void* ptr, unsigned long new_size)
{
  if (!ptr) return malloc(new_size);
  if (new_size == 0) { free(ptr); return NULL; }

  header_block_t* old = (header_block_t*)ptr - 1;
  if (old->size >= new_size)
  {
    old->size = new_size;
    return ptr;
  }

  size_t cur_sz = old->size;
  header_block_t* cur = old->next;
  for (; cur_sz < new_size && cur && cur->free; cur = cur->next)
    cur_sz += cur->size + sizeof(header_block_t);

  if (cur_sz >= new_size)
  {
    old->size = cur_sz;
    old->next = cur;
    return ptr;
  }

  void* res = malloc(new_size);
  if (!res) return NULL;

  size_t cpsz = (old->size > new_size ? new_size : old->size);
  memcpy(res, ptr, cpsz);
  free(ptr);

  return res;
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, int offset)
{
  return (void*)syscall(SYS_MMAP, addr, length, prot, flags, fd, offset);
}

int munmap(void* addr, size_t len)
{
  return (int)syscall(SYS_MUNMAP, addr, len);
}

