# Veldanava Language Rules

## Philosophy
Low-level/mid-level hybrid language. Systems programming + powerful scripting.
- Block termination: indent-based only (no `{}`)
- Block end: mandatory `;`
- Statement termination: mandatory `;`

## Keywords

### Core Keywords

| Category | Keywords |
|----------|----------|
| Control Flow | if, else, elif, while, for, break, continue, return |
| Variables | let, const |
| Types | i32, i64, u32, u64, f32, f64, bool, void, char, string |

### Permission-Based Keywords (God Mode)

| Keyword | Description |
|---------|-------------|
| `Primordial_Regalia` | Bắt buộc ở đầu file để kích hoạt God Mode. Cho phép sử dụng genesis, Incorporate, Sanction. |
| `genesis` | Tạo thực thể (thay thế func/class/struct/var). Yêu cầu Primordial_Regalia. |
| `Incorporate` | Import modules/thư viện. Yêu cầu Primordial_Regalia. |
| `Sanction` | Khối khai báo operator toàn file (hiện parser chỉ thu thập danh sách; enforcement có thể chưa đầy đủ ở backend). Yêu cầu Primordial_Regalia. |

## Indent-based Syntax (Python-style)

### Standard Syntax
```veldanava
let i32 x = 42;
return x;

if x > 0:
    let i32 y = 1;
;

while true:
    let i32 a = 1;
    break;
;

for i in range(10):
    let i32 b = i;
;
```

- Variable declaration: `let <type> <name> = <value>;` (type-first syntax)
- For iterator: type inferred from range call
- All blocks end with mandatory `;`

### Genesis Syntax (God Mode)

```veldanava
Primordial_Regalia;

genesis func main():
    genesis let i32 n = 0;
    genesis while n < 3:
        genesis print(n);
        genesis let i32 n = n + 1;
    ;
;

genesis func add(a, b):
    return a + b;
;

Incorporate "math", io;

Sanction:
    plus;
    minus;
    multi;
    div;
;
```

- `Primordial_Regalia;` bắt buộc ở đầu file
- **Declarations trong genesis scope cần `genesis` prefix** (let, func, class, struct)
- Control flow (if/while/for) và function calls/function declarations không cần genesis prefix
- `Incorporate` import modules (string identifier hoặc bare identifier)
- `Sanction` block whitelist toán tử số học (plus, minus, multi, div, power, root)

## Built-in Functions
- `range(n)` — tạo iterator từ 0 đến n-1
- `print(val)` — in giá trị ra stdout

## Statement Rules
- All statements end with `;` (required)
- Expression statements: `expr;`
- Empty statement: `;`
- Optional `()` for conditions in if/while

## Build & Run

```bash
cmake -B build && cmake --build build
./build/veldanc
```