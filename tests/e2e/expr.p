fn printi32(i32 x) -> void;

fn main() -> void
{
  i32 a = 1;
  i32 b = 2;
  i32 c = 3;
  i32 d = 4;
  i32 e = 5;
  i32 f = 6;

  // a += b += c * ++d - e-- + f;
  a += (b += (c * ++d)) - (e-- + --f) * (a = (b * 2));
  printi32(a);
  printi32(b);
  printi32(c);
  printi32(d);
  printi32(e);
  printi32(f);

  return 0;
}
