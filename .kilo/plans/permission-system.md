# Plan: Permission-Based Feature Gating for Veldanava

## Overview
Implement a "permission-based metaprogramming" system where language features (variables, functions, OOP, operators, imports) require explicit `allow` declarations before use. By default, the language is in a "locked" state — usage without declared permission is a compile-time error.

## Design Decisions

### 1. Permission Scope
- **File-level**: Permissions declared at the top of a file apply to the entire file.
- This keeps initial implementation simple. Block/function-level permissions can be added later.

### 2. Default Policy
- **Deny-by-default**: All features blocked unless explicitly allowed.
- Current codebase works because everything is implicitly allowed; this change makes it explicit.

### 3. Naming
The user provided placeholder names (`<sáng_tạo>`, `i_allow_you_to`). The user explicitly asked to find a "tên ngầu" (cool name). Suggestions:
- `manifest` instead of `<sáng_tạo>` — short, imperative, implies declaring intent
- `grant` instead of `i_allow_you_to` — matches "permission" metaphor closely
- Or keep user's names but formalize syntax

**Proposed syntax (recommended)**:
```veldanava
manifest creative;   # allows: variables, func, class/struct, operators
grant import;        # allows: import/include statements
```

### 4. Mechanism
- New AST node: `PermissionDeclNode` with fields `category` (creative | import | op | etc.) and `flags` (bitmask)
- Permissions are tracked in `ProgramNode` as a `std::vector<PermissionDeclNode>`
- Semantic analysis pass (in `ownership.cpp` or new `semantic.cpp`) validates:
  - Every non-trivial statement references an allowed category
  - Operators in expressions require `op` permission
  - Import statements require `import` permission

## Implementation Phases

### Phase 1: Lexer/Keywords (Low Risk)
**Files**: `src/frontend/lexer/token.h`, `src/frontend/lexer/keyword_table.h`, `src/frontend/lexer/lexer.cpp`

- Add `TokenType::Manifest`, `TokenType::Grant`
- Add keyword mappings: `manifest` → `Manifest`, `grant` → `Grant`
- (Optional) Add `TokenType::Import` for future import keyword

### Phase 2: AST Extension
**Files**: `src/frontend/ast/ast.h`

- Add `enum class PermissionCategory { Creative, Import, Op, All }`
- Add `struct PermissionDeclNode : Node` with:
  - `PermissionCategory category`
  - `uint32_t flags` (bitmask for granular control)
  - `std::vector<std::string> targets` (optional: list specific things to allow, e.g., `manifest func, class`)

### Phase 3: Parser Support
**Files**: `src/frontend/parser/parser.cpp`, `src/frontend/parser/parser.h`

- In `declaration()`, detect `manifest` and `grant` as top-level declarations
- Parse category identifier/name and optional target list
- Append `PermissionDeclNode` to `ProgramNode`
- No permission parsing inside `block_body` yet (file-level only in Phase 1)

### Phase 4: Semantic Analysis / Enforcement
**Files**: `src/middle/ownership/ownership.cpp` (or create `src/middle/semantic/semantic.cpp`)

- Pass 1: Collect all declared permissions from AST into a bitmask
- Pass 2: Walk AST and for each node:
  - `FuncDefNode`, `VarDeclNode`, `ClassDeclNode` → check `Creative` permission
  - `BinaryExprNode` (any op), `UnaryExprNode` → check `Op` permission
  - Future `ImportStmt` → check `Import` permission
  - Throw `std::runtime_error` with descriptive message if permission missing

### Phase 5: Basic Import Syntax (Separate Mini-Phase)
**Files**: `src/frontend/lexer/`, `src/frontend/parser/`, `src/backend/`

- Add `import <module>;` statement parsing (reuse permission `Grant`)
- Codegen: For MVP, import can be a no-op or emit a simple instruction. Real module loading is future work.

### Phase 6: Tests & Documentation
- Update `tests/test01.veldanava` and others to include required `manifest` declarations
- Add `tests/test_permissions.veldanava` demonstrating error cases (e.g., func without manifest)
- Update `docs/LANGUAGE_RULES.md` with permission system docs

## Risk Assessment
| Risk | Level | Mitigation |
|------|-------|------------|
| Breaking change: existing code needs `manifest` | HIGH | Phase 1-4 touch every feature. Plan migration of existing tests. |
| Syntax ambiguity: `manifest` vs variable name | LOW | `manifest` is a new keyword, scanned before identifier. |
| Scope confusion | LOW | Start with file-level only, document clearly. |
| User naming preference | MEDIUM | Confirm names (manifest/grant vs user's suggestions) before implementing. |

## Open Questions
1. **Naming**: Should we use the user's Vietnamese names (`sáng_tạo`, `i_allow_you_to`) or recommend English alternatives (`manifest`, `grant`)?
2. **Scope**: File-level acceptable, or should it support function/block-level from the start?
3. **Error style**: Hard error at parse time, or soft warning then error at semantic analysis?
4. **Granularity**: `manifest` allows ALL creative features by default, or should it be `manifest func` / `manifest class` separately?
