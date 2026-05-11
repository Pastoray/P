fn printi32(i32 x) -> void;
fn printi64(i64 x) -> void;

union Un
{
  i32 v1;
  i64 v2;
};

fn main() -> void
{
  Un u;

  u.v1 = 42;
  printi32(u.v1);

  u.v2 = 1000000; 
  printi64(u.v2);
  
  printi32(u.v1); 

  Un* ptr = &u;
  ptr->v1 = 555;
  printi32(u.v1);
  printi64(u.v2);

  u.v2 = -1;
  printi32(u.v1);

  return 0;
}
