# Plan: Permission-based Keywords System

## Overview
Thêm hệ thống keywords mới cho Veldanava nhằm hỗ trợ khai báo và kiểm soát quyền hạn (capabilities) khi sử dụng tài nguyên.

## Phân tích hiện trạng

### Keywords hiện có (keyword_table.h)
- **Control flow:** if, else, elif, while, for, break, continue, return
- **Declarations:** let, const, func, class, struct
- **Types:** i32, i64, u32, u64, f32, f64, bool, string, char, void
- **Logic:** and, or, not
- **Legacy (chưa implement):** manifest, grant, import

### AST hiện có (ast.h)
- Có `OwnershipInfo` với `is_mutable`, `is_borrowed`, `scope_level`
- Chưa có node cho permission/capability

## Keywords mới (đã đổi tên)

### Permission Flow
**QUAN TRỌNG:** `Primordial_Regalia` phải có đầu file, nếu thiếu → compiler error

### 1. `genesis` - Creation Authority (REPLACE func/class/struct)
**Mục đích:** Tạo thực thế, THAY THẾ hoàn toàn `func`, `class`, `struct`.

**Cú pháp:**
```veldanava
Primordial_Regalia;  // BẮT BUỘC đầu file

genesis func UltimateSkill(a, b):
    return a + b;
;

genesis class TrueDragon:
    // class body
;

genesis var WorldSeed = 42;
```

**Behavior:** Không có `Primordial_Regalia` → compiler reject các statement này.

### 2. `Incorporate` - Assimilation Authority
**Mục đích:** Import modules/thư viện.

**Cú pháp:**
```veldanava
Incorporate "UltimateCore";
Incorporate System.Manipulation;
Incorporate math, io, network;
```

### 3. `Sanction` - Decree Authority (BLOCK syntax)
**Mục đích:** Whitelist toán tử cho toàn file (global).

**Cú pháp block:**
```veldanava
Sanction:
    plus;
    minus;
    multi;
    div;
    power;
    root;
;
```

**Quan hệ với code hiện có:**
- Compiler cấm dùng +, -, *, / nếu chưa Sanction
- Syntax giống if/else block (indent-based)

### 4. `Primordial_Regalia` - Core Activation (BUILT-IN)
**Mục đích:** Unlock God Mode, active Genesis/Incorporate/Sanction.

**Cú pháp:**
```veldanava
Primordial_Regalia;  // Ở đầu file/block chính
```

## Các bước triển khai

### Phase 1: Cleanup
- [ ] Remove `grant`, `manifest`, `import` khỏi keyword_table.h
- [ ] Xóa Legacy TokenTypes

### Phase 2: Lexer
- [ ] Thêm TokenTypes: `Primordial_Regalia`, `Genesis`, `Incorporate`, `Sanction`
- [ ] Thêm operator identifiers: `Plus`, `Minus`, `Multi`, `Div`, `Power`, `Root`
- [ ] Cập nhật keyword_table.h

### Phase 3: AST
- [ ] Thêm `GenesisDeclNode` (kind: func/class/var, name, body)
- [ ] Thêm `IncorporateNode` (modules list)
- [ ] Thêm `SanctionBlockNode` (operators list)

### Phase 4: Parser
- [ ] Check `Primordial_Regalia` ở đầu file
- [ ] Thêm parse rule cho `genesis` (thay thế func/class/struct)
- [ ] Thêm parse rule cho `Incorporate`
- [ ] Thêm parse rule cho `Sanction` block

### Phase 5: Validation
- [ ] Compiler reject Genesis/Incorporate/Sanction nếu thiếu Primordial_Regalia
- [ ] Compiler reject dùng toán tử nếu chưa có Sanction