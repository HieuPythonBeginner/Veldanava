# Plan: Permission-based Keywords System

## Current status (2026-06-16)

- Git working tree clean, latest commit: `0a96345 feat: enforce sanctions and migrate syntax tests`
- All 29 tests pass: `SUMMARY total=29 pass=29 fail=0`

### Implemented

- `Primordial_Regalia;` is required at file start.
- `genesis` is used as the God Mode declaration/statement prefix.
- `Incorporate` is parsed as a God Mode statement.
- `Sanction` is parsed as an uppercase flat whitelist block and enforced by semantic validation for arithmetic operators and non-built-in calls.
- `if (cond):` and `while (cond):` are rejected.
- Logical keywords `and`, `or`, `not` are implemented.
- Backend has minimal module tracking for `Incorporate`.
- Math functions check requirement but no real semantics implemented.

### Not implemented yet

- Backend enforcement for `Sanction` can still be expanded beyond current operator/call validation.
- Module loading for `Incorporate` - no real file/module reading.
- Ownership/semantic pass; `ownership::init()` is currently a no-op.
- Full class/struct backend support.
- `elif` is recognized by lexer but not implemented as a statement.
- Function call backend is incomplete (no function table, ABI, return semantics).

## Current keywords

### Control flow

`if`, `else`, `elif`, `while`, `for`, `break`, `continue`, `return`

### Declarations and variables

`func`, `class`, `struct`, `var`, `let`, `const`

### Types

`i32`, `i64`, `u32`, `u64`, `f32`, `f64`, `bool`, `string`, `char`, `void`

### Logic

`and`, `or`, `not`

### Permission-based

- `Primordial_Regalia`
- `genesis`
- `Incorporate`
- `Sanction` uppercase is the only accepted form.

## Current syntax

### File header

```veldanava
Primordial_Regalia;
```

### Genesis function

```veldanava
Primordial_Regalia;

genesis func main():
    genesis print("hello");
;
```

### Variable declaration

```veldanava
genesis let i32 x = 42;
genesis let bool flag = true;
genesis let string name = "veldanava";
```

### Incorporate

```veldanava
Primordial_Regalia;

genesis Incorporate "math", io;
```

### Sanction

Flat syntax only:

```veldanava
Primordial_Regalia;

Sanction:
    plus;
    myfunc();
    myclass();
;
```

Rejected section syntax:

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

Entry rules:

- `Ident;` = operator-style sanction.
- `Ident();` = callable/class-style sanction.

Duplicate rejection (same shape only):

- `plus;` + `plus();` allowed (different shapes).
- `plus;` + `plus;` error.
- `myfunc();` + `myfunc();` error.

### Logical keywords

```veldanava
if a and b:
if a or b:
if not b:
```

### If/While (no parentheses)

```veldanava
if x > 0:
    genesis print("positive");
;

while x < 3:
    genesis print(x);
;
```

Wrong:

```veldanava
if (x > 0):
;
```

## Implementation phases

### Phase 1: Cleanup

Completed:

- Removed old permission naming from active syntax decisions.
- Current parser uses `Primordial_Regalia`, `genesis`, `Incorporate`, and `Sanction`.

### Phase 2: Lexer

Completed:

- `Primordial_Regalia`
- `Genesis`
- `Incorporate`
- `Sanction` is uppercase only.
- `elif` maps to `TokenType::Elif`; it is recognized but not implemented as a statement yet.
- `and`, `or`, and `not` map to parser/codegen-supported logical tokens.

### Phase 3: AST

Completed:

- `GenesisDeclNode`
- `IncorporateNode`
- `SanctionBlockNode`

### Phase 4: Parser

Completed:

- `Primordial_Regalia` must appear at file start.
- Top-level God Mode declarations require `genesis`.
- `Sanction` accepts uppercase flat entries only.
- Sectioned `operators/funcs/oop` syntax is rejected.
- Duplicate sanction names are rejected within the same shape.
- `if (cond):` and `while (cond):` are rejected.

Completed in validation:

- `validate_sanctions(ast::ProgramNode*)` enforces operator/call sanctions.
- Built-in `print()` and `range()` are exempt from sanction requirement.

Still open:

- Semantic validation pass beyond sanctions.
- Real module loading for `Incorporate`.
- Ownership/borrowing checks.

## Next priorities

1. Tách test runner positive/negative.
2. Implement `elif`.
3. Implement ownership/semantic pass.
4. Cải thiện class/struct backend.
5. Cải thiện function call backend.
6. Làm module loading thật cho `Incorporate`.
7. Mở rộng sanction policy sang backend/VM.
8. Thêm type checker.