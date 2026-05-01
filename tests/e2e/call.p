
fn printi32(i32 x) -> void;

fn fib(i32 x) -> i64
{
  if (x <= 1) { return x; }
  return fib(x - 1) + fib(x - 2);
}

fn add16(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g, i32 h, i32 i, i32 j, i32 k, i32 l, i32 m, i32 n, i32 o, i32 p) -> i64
{
  return a + b + c + d + e + f + g + h + i + j + k + l + m + n + o + p;
}

fn add4(i32 x, i32 y, i32 z, i32 w) -> i64
{
  return x + y + z + w;
}

fn main() -> void
{
  // 13
  printi32(fib(7));
  // 10
  printi32(add4(1, 2, 3, 4));
  // 136
  printi32(add16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
}

