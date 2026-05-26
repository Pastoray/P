fn printi32(i32 x) -> void;
fn printi64(i64 x) -> void;
fn malloc(u32 x) -> void*;
fn free(void* x) -> void;

struct Some
{
  i16 d1;
  i64 d2;
};

fn main() -> void
{
  Some* sm = malloc(10);
  sm->d1 = 16;
  sm->d2 = 64;

  printi32(sm->d1);
  printi64(sm->d2);

  free(sm);
  return 0;
}
