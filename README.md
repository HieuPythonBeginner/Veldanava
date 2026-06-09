# Veldanava

Low-level/mid-level hybrid language. Systems programming + powerful scripting.

## Build
```bash
cmake -B build -S .
cmake --build build --target veldanc
```

## Run
```bash
./build/veldanc
```

## Syntax (indent-based)
```veldanava
func main() -> i32:
    let x: i32 = 42;
    return x;
;

if x > 0:
    let y: i32 = 1;
;

while true:
    let x: i32 = 1;
    break;
;
```