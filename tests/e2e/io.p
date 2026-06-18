fn sys_open(char* filename, i32 flags, i32 mode) -> i64;
fn sys_write(i32 fd, void* buf, u64 count) -> i64;
fn sys_close(i32 fd) -> i64;
fn sys_read(i32 fd, void* buf, u64 count) -> i64;

fn printstr(char* text) -> void;
fn strlen(char* text) -> i64;
fn sys_exit(i32 status) -> void;
fn malloc(u64 size) -> void*;

i32 O_RDONLY = 0;
i32 O_WRONLY = 1;
i32 O_CREAT = 64;
i32 O_TRUNC = 512;

fn main() -> void
{
  char* filename = "test.txt";
  char* write_msg = "Hello from the Disk! 💾";
  
  i32 flags = O_WRONLY + O_CREAT + O_TRUNC;
  i64 fd_write = sys_open(filename, flags, 438);
  if (fd_write < 0)
  {
    sys_exit(1);
  }
  
  sys_write(fd_write, write_msg, strlen(write_msg));
  sys_close(fd_write);

  char* read_buf = malloc(32);
  i64 fd_read = sys_open(filename, O_RDONLY, 0);
  if (fd_read < 0)
  {
    sys_exit(2);
  }
  
  i64 bytes_read = sys_read(fd_read, read_buf, strlen(write_msg));
  sys_close(fd_read);
  
  *(read_buf + bytes_read) = 0;

  printstr(read_buf);
  sys_exit(0);
}
