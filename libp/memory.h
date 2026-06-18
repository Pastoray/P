#ifndef MEMORY_H
#define MEMORY_H

#include "defs.h"

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20

void* memset(void*, int, size_t);
void* memcpy(void*, const void*, size_t);
void* memmove(void*, const void*, size_t);
void* realloc(void*, unsigned long);
void* mmap(void*, size_t, int, int, int, int);
int munmap(void*, size_t);

#endif // MEMORY_H

