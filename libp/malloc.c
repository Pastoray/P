#include "malloc.h"
#include "defs.h"
#include "thread.h"
#include "sys_wrapper.h"

static header_block_t s_heap_dummy = { 0, false, NULL };
static header_block_t* s_heap_head = &s_heap_dummy;

volatile int g_heap_lock = 0;

void lock_heap()
{
  while (__atomic_test_and_set(&g_heap_lock, __ATOMIC_ACQUIRE))
  {
    syscall(SYS_FUTEX, &g_heap_lock, FUTEX_WAIT, 1, NULL);
  }
}

void unlock_heap()
{
  __atomic_clear(&g_heap_lock, __ATOMIC_RELEASE);
  syscall(SYS_FUTEX, &g_heap_lock, FUTEX_WAKE, 1, NULL);
}

void* sbrk(const int32_t inc)
{
  void* curr_brk = (void*)syscall(SYS_BRK, 0);
  const void* new_brk = (void*)syscall(SYS_BRK, (char*)curr_brk + inc);

  if (new_brk != (char*)curr_brk + inc)
    return NULL;

  return curr_brk;
}

header_block_t* alloc_header_block(const size_t sz)
{
  const size_t total_size = sz + sizeof(header_block_t);
  void* curr_brk = (void*)syscall(SYS_BRK, 0);

  void* new_brk = (void*)syscall(SYS_BRK, (char*)curr_brk + total_size);
  if (new_brk != (char*)curr_brk + total_size)
    return NULL;

  header_block_t* b = (header_block_t*)curr_brk;
  return b;
}

void* malloc(const size_t sz)
{
  size_t asz = (sz + 15) & ~15;

  lock_heap();
  header_block_t* curr = s_heap_head;
  header_block_t* prev = NULL;
  while(curr && (!curr->free || curr->size < asz))
  {
    prev = curr;
    curr = curr->next;
  }

  if (curr == NULL)
  {
    char* cbrk = (char*)syscall(SYS_BRK, 0);
    char* acbrk = (char*)(((unsigned long long)cbrk + 15) & -15);
    cbrk += acbrk - cbrk;
    syscall(SYS_BRK, cbrk);

    header_block_t* b = alloc_header_block(asz);
    if (b == NULL)
    {
      unlock_heap();
      syscall(SYS_EXIT, -2);
      return NULL;
    }

    b->size = asz;
    b->free = false;
    b->next = NULL;

    prev->next = b;
    unlock_heap();
    return (void*)(b + 1);
  }

  curr->free = false;
  unlock_heap();
  return (void*)(curr + 1);
}

void free(const void* p)
{
  if (!p) return;

  lock_heap();
  header_block_t* curr = (header_block_t*)p - 1;
  curr->free = true;
  unlock_heap();
}
