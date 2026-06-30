fn printi32(i32 x) -> void;
fn printstr(char* x) -> void;

namespace Lib
{
  i32 i = 0;
  i32 x = 16 + 4 * i + 1 / 3;
}

namespace Lib
{
  i32 hmm = 48 + 98 + Lib::x;
}

namespace Lib
{
  namespace Inner
  {
    i32 sin = 123;
  }

  i32 more = 456;
  namespace Inner
  {
    i32 x = 84 + more;
  }
}

namespace Another
{
  i32 x = 1 + 2 + Lib::more + Lib::Inner::sin;
  fn done(i32 x, i64 y) -> void
  {
    printstr("Done..\n");
    printi32(x + y);
  }
}

Lib::Inner::sin++;

fn main() -> void
{
  i32 i = 22;
  i += 3;
  Lib::i += 2;
  Lib::x--;

  printi32(i);
  printi32(Lib::i);
  printi32(Lib::hmm);
  printi32(Lib::x);
  printi32(Lib::more);
  printi32(Lib::Inner::sin);
  printi32(Lib::Inner::x);
  printi32(Another::x);
  Another::done(42, 58);

  return 0;
}
