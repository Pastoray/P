#ifndef DEFS_H
#define DEFS_H

#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_MMAP 9
#define SYS_MUNMAP 11
#define SYS_BRK 12
#define SYS_RT_SIGACTION 13
#define SYS_RT_SIGRETURN 15
#define SYS_NANOSLEEP 35
#define SYS_CLONE 56
#define SYS_EXECVE 59
#define SYS_EXIT 60
#define SYS_WAIT4 61
#define SYS_FUTEX 202
#define NULL ((void*)0)

#define bool _Bool
#define true 1
#define false 0

typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;

#endif // DEFS_H
