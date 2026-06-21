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
- `elif` is fully implemented with codegen, AST, and parser support.
- Legacy tests were migrated to current syntax; full `tests/*.veldanava` suite passes.
- `if (cond):` and `while (cond):` are rejected; use `if cond:` and `while cond:`.
- Docs updated to current syntax.
- Real class/struct backend is implemented for field layout, field access, constructors, and methods.
- Pointer/reference support is implemented across lexer/parser/AST, typechecker, ownership checks, codegen, and VM.

## Remaining

- Migrate legacy tests to current syntax.
  - `Sanction` uppercase only
  - flat sanction entries
  - duplicate sanction rejection
  - sectioned sanction rejection
  - `if` / `while` parentheses rejection
  - current `genesis` prefix rules

## System language foundation gaps

Implement these missing basics before treating Veldanava as a real system language:

1. ~~Real assignment expressions~~ ✅ COMPLETE
   - Assignment parsing implemented in `Veldanava/compiler/src/frontend/parser/parser.cpp`.
   - AST/codegen support added in `Veldanava/compiler/src/frontend/ast/ast.h` and `Veldanava/compiler/src/backend/codegen/codegen.cpp`.
   - `assignment()` now properly parses `=` and creates `BinaryExprNode` with `BinaryOp::Assign`.

2. ~~Complete `elif` codegen~~ ✅ COMPLETE
   - `elif_chain` is fully implemented in `Veldanava/compiler/src/backend/codegen/codegen.cpp`.
   - Parser already stores `elif_chain` in `Veldanava/compiler/src/frontend/ast/ast.h`.
   - Codegen properly processes elif conditions with condition jumps and body generation.

3. ~~Array and slice support~~ ✅ COMPLETE
   - `ArrayLit` and `IndexExpr` AST nodes added in `Veldanava/compiler/src/frontend/ast/ast.h`.
   - Lexer token support (`LBracket`/`RBracket`) and parser support for array literals and index expressions in `Veldanava/compiler/src/frontend/parser/parser.cpp`.
   - Codegen emits `ARR_NEW`, `ARR_GET`, `ARR_SET`, `ARR_LEN` opcodes in `Veldanava/compiler/src/backend/codegen/codegen.cpp`.
   - VM runtime handles array allocation, get/set, and length in `Veldanava/compiler/src/backend/vm/vm.cpp`.

4. ~~Real `char` literal support~~ ✅ COMPLETE
   - `scan_string()` in `Veldanava/compiler/src/frontend/lexer/lexer.cpp` now returns `TokenType::CharLit` for single-quoted literals.
   - Parser creates `LiteralNode` with `LiteralKind::Char` in `Veldanava/compiler/src/frontend/parser/parser.cpp`.
   - Codegen already handled `LiteralKind::Char` in `Veldanava/compiler/src/backend/codegen/codegen.cpp`.

5. Compound assignment and increment operators
   - Add `+=`, `-=`, `*=`, `/=`, `%=`, `++`, `--`.
   - Main files: `Veldanava/compiler/src/frontend/lexer/lexer.cpp`, `Veldanava/compiler/src/frontend/lexer/token.h`, `Veldanava/compiler/src/frontend/parser/parser.cpp`, `Veldanava/compiler/src/backend/codegen/codegen.cpp`.

6. Bitwise operators
   - Add `&`, `|`, `^`, `~`, `<<`, `>>` without breaking logical `and` / `or` / `not`.
   - Main files: `Veldanava/compiler/src/frontend/lexer/lexer.cpp`, `Veldanava/compiler/src/frontend/lexer/token.h`, `Veldanava/compiler/src/frontend/ast/ast.h`, `Veldanava/compiler/src/backend/codegen/codegen.cpp`, `Veldanava/compiler/src/backend/vm/instruction.h`, `Veldanava/compiler/src/backend/vm/vm.cpp`.

7. `switch` / `match`
   - Add parser, AST, codegen, and tests.
   - Main files: `Veldanava/compiler/src/frontend/parser/parser.cpp`, `Veldanava/compiler/src/frontend/ast/ast.h`, `Veldanava/compiler/src/backend/codegen/codegen.cpp`, `tests/`.

8. Error/result/option model
    - Add language syntax and runtime behavior for `Result<T, E>`, `Option<T>`, `panic`, and recovery if desired.
    - Main files: `Veldanava/compiler/src/frontend/ast/ast.h`, `Veldanava/compiler/src/frontend/parser/parser.cpp`, `Veldanava/compiler/src/middle/typechecker/typechecker.cpp`, `Veldanava/compiler/src/backend/vm/vm.cpp`.

9. Real module system
    - Replace minimal `Incorporate` behavior with namespace/export/import rules.
    - Main files: `Veldanava/compiler/src/frontend/module/module_loader.cpp`, `Veldanava/compiler/src/frontend/module/module_loader.h`, `Veldanava/compiler/src/frontend/parser/parser.cpp`, `Veldanava/compiler/src/backend/codegen/codegen.cpp`, `modules/`.

10. Correct typechecker
    - Make identifier type inference return variable/type info, not identifier names.
    - Add function signature lookup and real call type checking.
    - Main files: `Veldanava/compiler/src/middle/typechecker/typechecker.cpp`, `Veldanava/compiler/src/middle/typechecker/typechecker.h`, `Veldanava/compiler/src/frontend/ast/ast.h`.

13. Stronger ownership and borrow checking
    - Turn `ownership::init()` from no-op into real validation.
    - Main files: `Veldanava/compiler/src/middle/ownership/ownership.cpp`, `Veldanava/compiler/src/middle/ownership/ownership.h`, `Veldanava/compiler/src/frontend/ast/ast.h`.

14. Runtime memory model
    - Add heap, allocator interface, stack frame model, and memory safety checks.
    - Main files: `Veldanava/compiler/src/backend/vm/vm.h`, `Veldanava/compiler/src/backend/vm/vm.cpp`, `Veldanava/compiler/src/backend/vm/instruction.h`.

15. FFI / `extern` / C ABI
    - Add syntax and backend support for calling external C/libc/kernel APIs.
    - Main files: `Veldanava/compiler/src/frontend/parser/parser.cpp`, `Veldanava/compiler/src/frontend/ast/ast.h`, `Veldanava/compiler/src/backend/codegen/codegen.cpp`, `Veldanava/compiler/src/backend/vm/vm.cpp`.

16. Const correctness
    - Enforce `const` in parser, typechecker, ownership, and codegen.
    - Main files: `Veldanava/compiler/src/frontend/parser/parser.cpp`, `Veldanava/compiler/src/frontend/ast/ast.h`, `Veldanava/compiler/src/middle/typechecker/typechecker.cpp`, `Veldanava/compiler/src/middle/ownership/ownership.cpp`, `Veldanava/compiler/src/backend/codegen/codegen.cpp`.

## Veldanava Hybrid todo (under Veldanava/hybrid/)

1. ~~Stub project structure, build files, and entry point~~ ✅ COMPLETE
2. Profile-driven tier promotion in `HybridRuntime`
   - Track call count and execution time per AST node.
   - Auto-promote from Tier 0 to higher tiers.
3. Tier 0: Full AST interpreter
   - Support ProgramNode, VarDecl, Literal, BinaryExpr, FuncDef, FuncCall.
4. Tier 1: Baseline JIT (bytecode IR + inline cache)
   - Convert hot AST to simple bytecode IR.
   - Cache function call targets by signature.
5. Tier 2: Optimized JIT
   - Type specialization from profile data.
   - Inline small functions.
6. Tier 3: Max (MMap executable memory, optional native emit)
