# Veldanava Developer Prompt

## Quy tắc làm việc
- **Output ngắn gọn**: ≤20 dòng mỗi message
- **Đọc file trước khi sửa**: Luôn dùng `read()` tool trước `edit()`
- **Không dùng bash để sửa code**
- **Build test sau mỗi batch sửa**: `cmake --build build --target veldanc 2>&1 | tail -5`

## Kiến trúc
```
src/
├── frontend/
│   ├── lexer/     ← Tokenizer + indent handling
│   ├── parser/    ← Parser → AST
│   └── ast/       ← AST node types
├── middle/
│   └── ownership/ ← OwnershipContext
└── backend/
    ├── vm/        ← VM interpreter + opcodes
    └── codegen/   ← AST → VM opcode
```

## Build & Run
```bash
cmake -B build -S .
cmake --build build --target veldanc
./build/veldanc
```

## Syntax indent-based
- Block bằng indent, kết thúc bằng `;`
- Keywords: `if/else/while/for/func/let/const/return`

## File tham khảo
1. `docs/LANGUAGE_RULES.md` — Cú pháp Veldanava
2. `src/frontend/ast/ast.h` — AST node types
3. `src/backend/codegen/codegen.cpp` — Codegen