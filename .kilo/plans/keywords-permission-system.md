# Plan: Permission-based Keywords System

## Current status

Implemented subset:

- `Primordial_Regalia;` is required at file start.
- `genesis` is used as the God Mode declaration/statement prefix.
- `Incorporate` is parsed as a God Mode statement.
- `sanction` is parsed as a lowercase flat whitelist block and enforced by semantic validation for arithmetic operators and non-built-in calls.
- `if (cond):` and `while (cond):` are rejected.

Not implemented yet:

- Backend enforcement for `sanction` can still be expanded beyond current operator/call validation.
- Minimal module tracking for `Incorporate`; known math functions require `genesis Incorporate "math";`.
- Ownership/semantic pass; `ownership::init()` is currently a no-op.
- Full class/struct backend support.
- Legacy tests have been migrated to current syntax.

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
- `sanction` / `Sanction` lexer compatibility

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

### Incorporate

```veldanava
Primordial_Regalia;

genesis Incorporate "math", io;
```

### Sanction

Flat syntax only:

```veldanava
Primordial_Regalia;

sanction:
    plus;
    myfunc();
    myclass();
;
```

Rejected section syntax:

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

Duplicate sanction names are rejected within the same shape. `plus;` and `plus();` are allowed because they are different shapes.

## Implementation phases

### Phase 1: Cleanup

Completed:

- Removed old permission naming from active syntax decisions.
- Current parser uses `Primordial_Regalia`, `genesis`, `Incorporate`, and `sanction`.

Still open:

- Remove stale legacy expectations from tests/docs where needed.

### Phase 2: Lexer

Completed:

- `Primordial_Regalia`
- `Genesis`
- `Incorporate`
- `Sanction`
- lowercase `sanction` maps to `Sanction` for compatibility.
- `elif` maps to `TokenType::Elif`; it is recognized but not implemented as a statement yet.
- `and`, `or`, and `not` map to parser/codegen-supported logical tokens.

Still open:

- Dedicated operator tokens for `plus`, `minus`, `multi`, `div`, `power`, `root` are not required for the current flat parser design.

### Phase 3: AST

Completed:

- `GenesisDeclNode`
- `IncorporateNode`
- `SanctionBlockNode` with `operators`, `funcs`, `oop`

Still open:

- Better semantic model for flat sanction entries if backend enforcement is added.
- Class/struct AST/codegen model.

### Phase 4: Parser

Completed:

- `Primordial_Regalia` must appear at file start.
- Top-level God Mode declarations require `genesis`.
- `sanction` accepts lowercase flat entries.
- Sectioned `operators/funcs/oop` syntax is rejected.
- Duplicate sanction names are rejected within the same shape.
- `if (cond):` and `while (cond):` are rejected.

Still open:

- Semantic validation pass.

### Phase 5: Validation

Completed:

- Parser rejects missing `Primordial_Regalia`.
- Parser rejects top-level declarations without `genesis`.
- Parser rejects sectioned sanction syntax.
- Parser rejects duplicate sanction names.

Still open:

- Enforce sanctioned operators/calls in expressions.
- Enforce `Incorporate` module availability.
- Enforce ownership/borrowing rules.

## Docs/tests follow-up

- Public docs now describe lowercase flat `sanction`, same-shape duplicate rejection, and logical keywords.
- Legacy tests have been migrated; `tests/*.veldanava` currently passes.
- Add focused negative tests if a test runner for expected-failure cases is added.
