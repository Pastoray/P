fn printf32(f32 x) -> void;
fn printf64(f64 x) -> void;

struct Node
{
  f32 f1;
  f64 f2;
  f32 f3;
};

fn main() -> void
{
  Node n1;
  Node n2;

  n1.f1 = 1.2;
  n1.f2 = 2.3;
  n1.f3 = 3.4;

  printf32(n1.f1 + n1.f2 + n1.f3);

  n2.f1 = 5.6;
  n2.f2 = 121212.56456;
  n2.f3 = 15.4884;

  f64 v = n2.f1 + n2.f2 - n2.f3;

  printf64(v);
  return 0;
}
