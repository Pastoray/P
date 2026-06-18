#ifndef THREAD_H
#define THREAD_H

#define CLONE_VM 0x00000100
#define CLONE_FS 0x00000200
#define CLONE_FILES 0x00000400
#define CLONE_SIGHAND 0x00000800
#define CLONE_THREAD 0x00001000

#define MAX_THREADS 64
#define THREAD_STACK_SIZE (64 * 1024)

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

typedef struct rt_thread
{
  int id;
  void* stack;
  volatile int done;
} rt_thread_t;

typedef struct rt_mutex
{
  volatile int state;
} rt_mutex_t;

void __thread_entry_wrapper(void (*)(), rt_thread_t*);
int spawn_thread(void (*)());
long thread_clone(unsigned long, void*, void (*)(void (*)(), rt_thread_t*), void (*)(), rt_thread_t*);
void join_thread(int);
void join_all_threads();
void mutex_lock(rt_mutex_t*);
void mutex_unlock(rt_mutex_t*);
void thread_sleep(long, long);
int exec_prog(const char*, const char* [], const char* []);
long wait_for_child(int, int*);
int set_sigact(int, void (*)(int));

#endif // THREAD_H
