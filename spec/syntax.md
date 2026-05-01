## Syntax Overview
- Semicolon terminated statements
- C-like blocks `{}`
- C-style variable typing `i8 x;`

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Bitwise: `<<`, `>>`, `&`, `|`, `^`, `~`
- Logical: `&&`, `||`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`

### Control Flow
> `()` are required, `{ ... }` omittable for 1 liner
- `if (cond1) { ... } else if (cond2) { ... } else { ... }`
- `while (cond) { ... }`
- `for (init; cond; step) { ... }`
- `break;`
- `continue;`

### Types
- Integer types: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`
- Float types: `f32`, `f64`
- Boolean: `bool` (`true`/`false`)
- Void: `void` (for functions that return nothing)

### Pointers
- Pointers: `i16* p = &x;`
- Dereference: `*p = 42;`
- Pointer arithmetic: enabled (`p + n` advances by `n * sizeof(*p)`)
- Null literal: `null`

### Structs
- Declaration: `struct name { ... };`
- Supports forward declaration (`struct name;`)
- May contain pointers to their own type
- May define associated methods
- 
### Functions
- Defined as:
```
fn func() -> T { ... }
```

### Enums
Enums are **strictly scoped**. Variants are not injected into the surrounding namespace.

```
enum Color { RED, GREEN }
Color c = Color::RED;
```
