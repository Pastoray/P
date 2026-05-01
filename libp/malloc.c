#include "malloc.h"
#include "defs.h"
#include "exit.h"

static header_block_t s_heap_dummy = { 0, false, NULL };
static header_block_t* s_heap_head = &s_heap_dummy;

void* set_brk(const void* new_brk)
{
  void* res;
  asm volatile (
    "movq %1, %%rax\n\t"
    "movq %2, %%rdi\n\t"
    "syscall\n\t"
    : "=r" (res)
    : "r" ((long)SYS_BRK), "r" (new_brk)
    : "rax", "rdi", "memory", "cc",
      "rcx", "r11"
  );
  return res;
}

void* sbrk(const int32_t inc)
{
  void* curr_brk = get_curr_brk();
  const void* new_brk = set_brk((char*)curr_brk + inc);

  if (new_brk != (char*)curr_brk + inc)
    return NULL;

  return curr_brk;
}

void* get_curr_brk(void)
{
  void* res;

  // Should never fail (we assume it doesn't)
  asm volatile (
    "movq %1, %%rax\n\t"
    "movq $0, %%rdi\n\t"
    "syscall\n\t"
    "movq %%rax, %0\n\t"
    : "=r" (res)
    : "r" ((long)SYS_BRK)
  );
  return res;
}

header_block_t* alloc_header_block(const size_t sz)
{
  const size_t total_size = sz + sizeof(header_block_t);
  void* curr_brk = get_curr_brk();

  void* new_brk = set_brk((char*)curr_brk + total_size);
  if (new_brk != (char*)curr_brk + total_size)
    return NULL;

  header_block_t* b = (header_block_t*)curr_brk;
  return b;
}

void* malloc(const size_t sz)
{
  header_block_t* curr = s_heap_head;
  header_block_t* prev = NULL;
  while(curr && (!curr->free || curr->size < sz))
  {
    prev = curr;
    curr = curr->next;
  }

  if (curr == NULL)
  {
    header_block_t* b = alloc_header_block(sz);
    if (b == NULL)
    {
      exit(-2);
      return NULL;
    }

    b->size = sz;
    b->free = false;
    b->next = NULL;

    prev->next = b;
    return (void*)(b + 1);
  }

  curr->free = false;
  return (void*)(curr + 1);
}

void free(const void* p)
{
  if (!p) return;

  header_block_t* curr = (header_block_t*)p - 1;
  curr->free = true;
}