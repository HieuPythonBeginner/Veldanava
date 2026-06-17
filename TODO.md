# TODO

## Completed

- Parser accepts uppercase `Sanction`.
- Parser accepts flat `Sanction` entries:
  ```veldanava
  Sanction:
      plus;
      myfunc();
      myclass();
  ;
  ```
- Parser rejects sectioned `Sanction` syntax:
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
- Duplicate sanction names are rejected within the same shape. `plus;` and `plus();` are allowed because they are different shapes.
- Lexer keyword mappings were fixed for `and`, `or`, `not`, and `elif`.
- Codegen now supports `and`, `or`, and `not` boolean-style expressions.
- `elif` is recognized but still not implemented as a statement.
- Legacy tests were migrated to current syntax; full `tests/*.veldanava` suite passes.
- `if (cond):` and `while (cond):` are rejected; use `if cond:` and `while cond:`.
- Docs updated to current syntax.

## Remaining

- Migrate legacy tests to current syntax.
- Semantic validation enforces `Sanction` for arithmetic operators and non-built-in calls.
- `Incorporate` has minimal module tracking; known math functions require `genesis Incorporate "math";`.
- Finish ownership/semantic pass; `ownership::init()` is currently a no-op.
- Improve class/struct AST and codegen support.
- Add focused tests for:
  - `Sanction` uppercase only
  - flat sanction entries
  - duplicate sanction rejection
  - sectioned sanction rejection
  - `if` / `while` parentheses rejection
  - current `genesis` prefix rules
