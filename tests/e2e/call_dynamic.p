fn printi32(i32 x) -> void;
fn func1(i32 x) -> i32
{
  return x;
}

fn func2(i32 x) -> i32
{
  return x * x;
}

fn func3(i32 x) -> i32
{
  return x * x * x;
}

fn main() -> void
{
  fn<(i32), i32> x[3];
  x[0] = func1;
  x[1] = func2;
  x[2] = func3;
  for (i32 i = 2; i >= 0; i--)
  {
    printi32(x[i](2));
  }
}
