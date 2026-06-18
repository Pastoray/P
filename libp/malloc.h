#ifndef MALLOC_H
#define MALLOC_H

#include "defs.h"

#define HEAP_SIZE (1024 * 64)
typedef struct __attribute__((aligned(16))) _header_block_t
{
  size_t size;
  bool free;
  struct _header_block_t* next;
} header_block_t;

void* sbrk(int32_t);
header_block_t* alloc_header_block(size_t);
void* malloc(size_t);
void free(const void*);
void lock_heap();
void unlock_heap();

#endif // MALLOC_H
