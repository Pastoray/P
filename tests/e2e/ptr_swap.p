fn printi32(i32 x) -> void;

fn main() -> void
{
  i32 x = 10;
  i32 y = 20;

  i32* px = &x;
  i32* py = &y;
  i32 tmp = *px;

  *px = *py;
  *py = tmp;

  printi32(x);
  printi32(y);

  return 0;
}


