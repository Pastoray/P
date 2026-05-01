
fn printi32(i32 x) -> void;

fn main() -> void
{
  i32 x = 1;
  if (x && 1)
  {
    i32 i = 2;
    for (; i <= 5; i++) { x *= i; }

    i64 y = i;
    for (i32 j = 6; j <= 15; j++)
    {
      if (j == 11) { continue; }
      else if (j == 12) { y = j; break; }
    }
    // 12;
    printi32(y);
    for (i32 i = 6; i <= 10; i++)
    {
      x *= i;
    }
    // 10! = 3628800
    printi32(x);
  }
  else { printi32(0); }
}
