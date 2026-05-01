#ifndef MALLOC_H
#define MALLOC_H

#include "defs.h"

#define HEAP_SIZE (1024 * 64)
typedef struct _header_block_t
{
  size_t size;
  bool free;
  struct _header_block_t* next;
} header_block_t;

// Increments current brk
// Returns previous brk (before increment)
void* sbrk(int32_t inc);

// Set brk to specified new_brk
void* set_brk(const void* new_brk);

// Get current brk
void* get_curr_brk(void);

// Allocate header_block_t
header_block_t* alloc_header_block(size_t sz);

// Allocate sz + sizeof(header_block_t) (metadata)
void* malloc(size_t sz);

// Free allocated block
void free(const void* p);

#endif // MALLOC_H
