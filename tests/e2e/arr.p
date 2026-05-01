fn printi32(i32 x) -> void;

fn main() -> void
{
  i32 x[6];
  x[0] = 67;
  *(x + 4) = 16;
  printi32(x[4]);
  for (i32 i = 0; i < 6; i++)
  {
    x[i] = i + 2;
  }

  for (i32 j = 0; j < 6; j++)
  {
    printi32(x[j]);
  }

  i32 l1 = 10;
  i32 l2 = 20;
  i32 y[l1][l2];

  y[l1 - 1][l2 - 1] = 1;
  printi32(y[l1 - 1][l2 - 1]);

  *(y[l1 - 1] + 4) = 67;
  printi32(y[l1 - 1][4]);

  for (i32 i = 0; i < l1; i++)
  {
    for (i32 j = 0; j < l2; j++)
    {
      y[i][j] = i + j;
    }
  }

  for (i32 i = 0; i < 3; i++)
  {
    for (i32 j = 0; j < 3; j++)
    {
      printi32(y[i][j]);
    }
  }

  return 0;
}


