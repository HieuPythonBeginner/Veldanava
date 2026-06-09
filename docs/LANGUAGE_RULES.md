# Veldanava Language Rules

## Philosophy
Low-level/mid-level hybrid language. Systems programming + powerful scripting.
- Block termination: indent-based only (no `{}`)
- Block end: mandatory `;`
- Statement termination: mandatory `;`

## Keywords (Phase 1)

| Category | Keywords |
|----------|----------|
| Control Flow | if, else, elif, while, for, break, continue, return |
| Functions | func |
| Variables | let, const |
| Types | i32, i64, u32, u64, f32, f64, bool, void, char, string |

## Indent-based Syntax (Python-style)

```veldanava
func main() -> i32:
    let x: i32 = 42;
    return x;
;

if x > 0:
    let y: i32 = 1;
;

while true:
    let a: i32 = 1;
    break;
;

# for-range loop
for i in range(10):
    let b: i32 = i;
;
```

## Built-in Functions
- `range(n)` — tạo iterator từ 0 đến n-1

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