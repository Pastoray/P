fn printstr(char* x) -> void;
fn printchar(char x) -> void;

fn main() -> void
{
  char* str = "Hello, World! ✨\n";
  printstr(str);

  printchar(str[0]);
  printchar(str[4]);
  printchar(str[2]);
  printchar(str[3]);
  printchar(str[4]);
  printchar(str[7]);
  printchar('s');
  printchar('\n');

  return 0;
}
