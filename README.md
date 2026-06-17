# Veldanava

Low-level/mid-level hybrid language. Systems programming + powerful scripting.

Current state: **experimental compiler/parser snapshot**. The parser, codegen, and VM can run small programs, but some language features are parsed only and not fully enforced yet.

## Build

```bash
cmake -B build -S .
cmake --build build --target veldanc
```

## Run

```bash
./build/veldanc <file.veldanava>
```

Example:

```bash
./build/veldanc tests/test_genesis_full.veldanava
```

## Syntax (indent-based)

Every file must start with:

```veldanava
Primordial_Regalia;
```

God Mode declarations use the `genesis` prefix:

```veldanava
Primordial_Regalia;

genesis func main():
    genesis let i32 n = 0;
    genesis while n < 3:
        genesis print(n);
        genesis let i32 n = n + 1;
    ;
;
```

Function calls inside a `genesis` block also use the `genesis` prefix:

```veldanava
Primordial_Regalia;

genesis func main():
    genesis print("hello");
;
```

Control-flow keywords are lowercase:

```veldanava
if x > 0:
    ...
;

while x < 10:
    ...
;

for i in range(10):
    ...
;
```

Parentheses are not allowed for `if` / `while` conditions. Use:

```veldanava
if x > 0:
    ...
;
```

not:

```veldanava
if (x > 0):
    ...
;
```

## Sanction

`Sanction` is uppercase and uses flat entries only:

```veldanava
Sanction:
    plus;
    myfunc();
```

- `Ident;` is an operator-style sanction entry.
- `Ident();` is a callable/class-style sanction entry.

```veldanava
Sanction:
    plus;
    myfunc();
    MyClass();
```

Current parser collects `SanctionBlockNode`; semantic validation enforces sanctioned arithmetic operators and non-built-in calls.

## Incorporate

`Incorporate` is parsed as a God Mode statement:

```veldanava
Primordial_Regalia;

genesis Incorporate "math", io;
```

Current backend tracks incorporated modules and rejects known module functions unless their module is incorporated.

## Built-in functions

- `print(value)`
- `range(n)`

## Known limitations

- `sanction` enforces arithmetic operators and non-built-in calls; real backend policy can still be expanded.
- `Incorporate` has minimal module tracking; known math functions require `genesis Incorporate "math";`.
- `ownership::init()` is currently a no-op.
- Class/struct syntax is partially parsed but not fully modeled in codegen.
- Legacy tests have been migrated to current syntax.
