fn printi32(i32 x) -> void;
fn printf32(f32 x) -> void;
fn printf64(f64 x) -> void;
f32 pi = 3.14;

fn main() -> void
{
  f64 spi = 3.1415;
  printf32(pi);
  printf64(spi);
  printf32(pi + spi);

  f32 some = 6.7 + pi - spi;
  printf64(some);
  if (true)
  {
    f64 x = 67;
    printf32(x);
  }
  return 0;
}
