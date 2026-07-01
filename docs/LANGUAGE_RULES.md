# Veldanava Language Rules

## Philosophy

Veldanava is a low-level/mid-level hybrid language with systems-programming roots and scripting-style syntax.

Core syntax rules:

- Indent-based blocks, similar to Python.
- No `{}` block delimiters.
- Every block ends with a mandatory `;`.
- Every statement ends with a mandatory `;`.
- Empty statement `;` is allowed at top level.

## Modes

Every `.veldanava` file is a Veldanava file. The compiler has 2 processing modes:

**Compiler mode (AOT):**
```veldanava
#Pre_Descent Primordial_Regalia

#Beginning_Eden
```
Xử lý bởi `veldanc`, biên dịch sang bytecode rồi chạy trên VM.

**Hybrid mode (JIT/Interpreter):**
```veldanava
no magic header
```
Tự động chuyển sang `Veldanava Hybrid`, dùng Tier 1 interpreter + Tier 2 JIT.

## Required file header

To use Veldanava compiler mode, the file must start with:

```veldanava
#Pre_Descent Primordial_Regalia

#Beginning_Eden
```

`#Pre_Descent Primordial_Regalia` is the entry token that selects compiler mode.
`#Beginning_Eden` unlocks the full feature set.

The standalone `Primordial_Regalia;` statement is no longer accepted.

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

| Keyword / Directive | Description |
|---|---|
| `#Pre_Descent Primordial_Regalia` | Line-1 magic header. Selects compiler mode and enables god mode keywords. |
| `#Beginning_Eden` | Enables basic programming features (`if`/`else`/`elif`, `while`, `for`, arrays, assignment, operators...). Requires god mode. |
| `genesis` | Prefix for God Mode declarations/statements. |
| `Incorporate` | Import/module declaration. Parsed only for now. |
| `Sanction` | Flat global whitelist block. Parsed only for now. |

`Sanction` uppercase is the only accepted keyword.

### Permission system

Veldanava dùng 2 flags khóa keyword:

| Flag | Được mở bởi | Cho phép |
|------|-------------|---------|
| `has_god_mode_` | `#Pre_Descent Primordial_Regalia` (line 1) | `genesis`, `Sanction`, `Incorporate`, và directive `#Beginning_Eden` |
| `has_basic_features_` | `#Beginning_Eden` (line 2) | `if/else/elif`, `while`, `for`, array, list, assignment, toán tử... |

### Permission-based keywords

| Keyword / Directive | Description |
|---|---|
| `#Pre_Descent Primordial_Regalia` | Line-1 magic header. Selects compiler mode and enables god mode keywords. |
| `#Beginning_Eden` | Enables basic programming features (`if`/`else`/`elif`, `while`, `for`, arrays, assignment, operators...). Requires god mode. |
| `genesis` | Prefix for God Mode declarations/statements. |
| `Incorporate` | Import/module declaration. Parsed only for now. |
| `Sanction` | Flat global whitelist block. Parsed only for now. |

`Sanction` uppercase is the only accepted keyword.

`Sanction` uses flat entries only:

```veldanava
#Pre_Descent Primordial_Regalia

#Beginning_Eden

Sanction:
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
Sanction:
    plus;
    plus;
;
```

```veldanava
Sanction:
    myfunc();
    myfunc();
;
```

Section syntax is rejected. Do not write:

```veldanava
Sanction:
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

Current status: `Sanction` is collected into `SanctionBlockNode` and semantic validation enforces arithmetic operators plus non-built-in calls.

## Control flow

### If

```veldanava
#Pre_Descent Primordial_Regalia

#Beginning_Eden

genesis func main():
    genesis if x > 0:
        genesis print("positive");
    ;
;
```

Parentheses are not allowed:

```veldanava
#Pre_Descent Primordial_Regalia

#Beginning_Eden

genesis func main():
    genesis if (x > 0):
        genesis print("invalid");
    ;
;
```

### While

```veldanava
#Pre_Descent Primordial_Regalia

#Beginning_Eden

genesis func main():
    genesis while x < 10:
        genesis print(x);
    ;
;
```

Parentheses are not allowed:

```veldanava
#Pre_Descent Primordial_Regalia

#Beginning_Eden

genesis func main():
    genesis while (x < 10):
        genesis print(x);
    ;
;
```

### For

```veldanava
#Pre_Descent Primordial_Regalia

#Beginning_Eden

genesis func main():
    genesis for i in range(10):
        genesis print(i);
    ;
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
#Pre_Descent Primordial_Regalia

#Beginning_Eden

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

`elif` is linked to `if`/`else` chains via the parser and codegen.

## Built-in functions

- `print(value)` prints a value to stdout.
- `range(n)` creates an integer iterator from `0` to `n - 1`.

## Unified compiler driver

`running_true_dragon_core` is the single entry point for the entire Veldanava ecosystem. It auto-routes by file extension:

```bash
./build/running_true_dragon_core <file>
```

A shorter alias `velrun` is also provided:

```bash
./velrun <file>
```

### Installation

For system-wide use (all users on the machine):

```bash
sudo cmake --build build --target install
```

This copies `velrun` to `/usr/local/bin/`, which is already in every user's PATH.

Alternatively, use the install script:

```bash
./install.sh
```

The script detects whether you have sudo access:
- **With sudo**: installs system-wide to `/usr/local/bin/`
- **Without sudo**: installs to `~/.local/bin/` and updates PATH if needed

| Extension | Binary | Description |
|-----------|--------|-------------|
| `.veldanava` | `build/veldanc` | Veldanava compiler (AOT mode if `#Pre_Descent` present, otherwise auto-routes to Veldanava Hybrid) |
| `.veldora` | `build-veldora/Veldora_Hybrid_AOT_JIT_Interpreter` | Veldora hybrid AOT/JIT/interpreter |
| `.velzard` | `build-velzard/velzard_Interpreter` | Velzard interpreter |
| `.velgrynd` | `build-velgrynd/velgrynd_JIT` | Velgrynd JIT |

Each language has its own dedicated compiler/runtime. `running_true_dragon_core` simply selects the correct backend from the file extension.

## Current limitations

- `Sanction` enforces arithmetic operators and non-built-in calls; real backend policy can still be expanded.
- `Incorporate` has minimal module tracking; known math functions require `genesis Incorporate "math";`.
- `ownership::init()` is currently a no-op.
- Class/struct codegen is incomplete.
- Legacy tests have been migrated to current syntax.
