fn set_sigact(i32 signum, void* handler) -> i32;
fn sys_exit(i32 status) -> void;
fn printstr(char* x) -> void;

i32 SIGSEGV = 11;

fn hand_sigsegv(i32 signum, void* info, void* ctx) -> void
{
  printstr("\n[SIGNAL INTERCEPTED] SIGSEGV Detect, Shutting down..\n");
  sys_exit(42);
}

fn main() -> void
{
  set_sigact(SIGSEGV, hand_sigsegv);

  i32* bad_ptr = 0; 
  *bad_ptr = 42; 

  sys_exit(0);
}
