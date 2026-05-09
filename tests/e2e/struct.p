fn printi32(i32 x) -> void;

struct Node
{
  i32 value;
  Node* next;
};

struct LinkedList
{
  Node* head;
  i32 size;
};

fn main() -> void
{
  Node n1;
  Node n2;
  LinkedList list;

  n1.value = 100;
  n1.next = 0;

  n2.value = 200;
  n2.next = &n1;

  list.head = &n2;
  list.size = 2;

  Node* first = list.head;
  Node* second = first->next;
  
  printi32(second->value);

  return 0;
}
