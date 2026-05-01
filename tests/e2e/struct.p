fn printi32(i32 x) -> ;

struct Player
{
  i32 id;
  i64 score;
  bool alive;
};

fn main() ->
{
  Player p;

  p.id = 0;
  p.score = 12;
  p.alive = true;
  p.alive = 0;

  printi32(p.id);
  printi64(p.score);
  printbool(p.alive);

  i64 idk = p.id + p.score;
  i32 some = (&p)->id + (&p)->score;

  printi64(idk + some);
  printi32(idk + some);

  return 0;
}


