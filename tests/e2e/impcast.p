fn printi32(i32 x) -> void;
fn printi64(i64 x) -> void;

fn main() -> void
{
  i8 small = 10;
  i32 medium = small;
  i64 large = medium;

  u32 unsigned_val = 4000;
  i64 signed_large = unsigned_val;

  i32 a = 5;
  i64 b = 100;
  i64 res = a + b;

  i32* ptr = &a;
  i32 val = 1;
  i32 x = ptr[val];

  printi32(medium);
  printi64(signed_large);
  printi64(res);

  return 0;
}
