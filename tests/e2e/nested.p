fn printi32(i32 x) -> void;

fn main() -> void
{
  i32 x = 10;
  if (x > 5)
  {
    if (x < 15)
    {
      if (x == 10)
      {
        printi32(1);
      }
      else
      {
        printi32(0);
      }
    }
    else
      {
        printi32(0);
      }
  }
  else
  {
    printi32(0);
  }
}
