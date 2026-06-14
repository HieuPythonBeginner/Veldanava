# Veldanava Language Rules

## Philosophy

Veldanava is a low-level/mid-level hybrid language with systems-programming roots and scripting-style syntax.

Core syntax rules:

- Indent-based blocks, similar to Python.
- No `{}` block delimiters.
- Every block ends with a mandatory `;`.
- Every statement ends with a mandatory `;`.
- Empty statement `;` is allowed at top level.

## Required file header

Every file must start with:

```veldanava
Primordial_Regalia;
```

Without this header, God Mode syntax is rejected.

## Keywords

### Core keywords

| Category | Keywords |
|---|---|
| Control flow | `if`, `else`, `elif`, `while`, `for`, `break`, `continue`, `return` |
| Variables | `let`, `const` |
| Declarations | `func`, `class`, `struct`, `var` |
| Types | `i32`, `i64`, `u32`, `u64`, `f32`, `f64`, `bool`, `void`, `char`, `string` |
| Logic | `and`, `or`, `not` |

### Permission-based keywords

| Keyword | Description |
|---|---|
| `Primordial_Regalia` | Required at the start of the file. Enables God Mode syntax. |
| `genesis` | Prefix for God Mode declarations/statements. |
| `Incorporate` | Import/module declaration. Parsed only for now. |
| `sanction` | Flat global whitelist block. Parsed only for now. |

`Sanction` uppercase is accepted by the lexer for compatibility, but the documented keyword is lowercase `sanction`.

## God Mode syntax

### Function

```veldanava
Primordial_Regalia;

genesis func main():
    genesis print("hello");
;
```

### Variable

```veldanava
Primordial_Regalia;

genesis func main():
    genesis let i32 x = 42;
;
```

### Class / struct

```veldanava
Primordial_Regalia;

genesis class MyDragon:
;

genesis struct MyData:
;
```

Class and struct declarations are parsed, but backend support is incomplete.

### Incorporate

```veldanava
Primordial_Regalia;

genesis Incorporate "math", io;
```

`Incorporate` accepts string literals or bare identifiers. The backend tracks incorporated modules and rejects known module functions unless their module is incorporated.

Known module functions currently include math functions such as `sqrt`, `sin`, `cos`, `tan`, `abs`, `floor`, `ceil`, and `pow`; they require:

```veldanava
genesis Incorporate "math";
```

## Sanction syntax

`sanction` uses flat entries only:

```veldanava
Primordial_Regalia;

sanction:
    plus;
    myfunc();
    myclass();
;
```

Entry rules:

| Entry shape | Meaning |
|---|---|
| `Ident;` | Operator-style entry, for example `plus;` |
| `Ident();` | Callable/class-style entry, for example `myfunc();` or `myclass();` |

Duplicate names are rejected within the same entry shape. `plus;` and `plus();` are allowed because they are different shapes. These are invalid:

```veldanava
sanction:
    plus;
    plus;
;
```

```veldanava
sanction:
    myfunc();
    myfunc();
;
```

Section syntax is rejected. Do not write:

```veldanava
sanction:
    operators:
        plus;
    funcs:
        myfunc();
    oop:
        myclass();
;
```

The parser rejects it with:

```text
Sanction block does not support operators/funcs/oop sections; use flat entries only
```

Current status: `sanction` is collected into `SanctionBlockNode` and semantic validation enforces arithmetic operators plus non-built-in calls.

## Control flow

### If

```veldanava
if x > 0:
    genesis print("positive");
;
```

Parentheses are not allowed:

```veldanava
if (x > 0):
    genesis print("invalid");
;
```

### While

```veldanava
while x < 10:
    genesis print(x);
;
```

Parentheses are not allowed:

```veldanava
while (x < 10):
    genesis print(x);
;
```

### For

```veldanava
for i in range(10):
    genesis print(i);
;
```

## Statement rules

- All statements end with `;`.
- Expression statements use `expr;`.
- Empty statements use `;`.
- `if` / `while` conditions must not use parentheses.
- Inside `genesis` blocks, statements normally require the `genesis` prefix.

Example:

```veldanava
Primordial_Regalia;

genesis func main():
    genesis if true:
        genesis print("ok");
    ;
;
```

## Logical expressions

Logical keywords are lowercase:

```veldanava
if a and b:
    genesis print("both");
;

if a or b:
    genesis print("either");
;

if not b:
    genesis print("not b");
;
```

`elif` is recognized by the lexer but not implemented as a statement yet; use nested `else` + `if`.

## Built-in functions

- `print(value)` prints a value to stdout.
- `range(n)` creates an integer iterator from `0` to `n - 1`.

## Build and run

```bash
cmake -B build -S .
cmake --build build --target veldanc
./build/veldanc <file.veldanava>
```

Smoke test:

```bash
./build/veldanc tests/test_genesis_full.veldanava
```

Expected output:

```text
0
1
2
[OK] Executed tests/test_genesis_full.veldanava
```

## Current limitations

- `sanction` enforces arithmetic operators and non-built-in calls; real backend policy can still be expanded.
- `Incorporate` has minimal module tracking; known math functions require `genesis Incorporate "math";`.
- `ownership::init()` is currently a no-op.
- Class/struct codegen is incomplete.
- Legacy tests have been migrated to current syntax.
