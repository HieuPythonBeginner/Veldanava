# Veldanava Quick Start

## 1. Build

```bash
cmake -B build -S .
cmake --build build --target veldanc
```

## 2. Install (optional)

```bash
# Cách 1: System-wide (cần sudo)
sudo cmake --build build --target install

# Cách 2: Auto-detect
./install.sh
```

## 3. Run file

```bash
# Direct (chỉ compiler mode)
./build/veldanc hello.veldanava

# Hoặc dùng unified driver (tự động theo extension)
./velrun hello.veldanava
./velrun hello.veldora
./velrun hello.velzard
./velrun hello.velgrynd
```

## 4. Cú pháp cơ bản

### File header (bắt buộc)

```veldanava
#Pre_Descent Primordial_Regalia
#Beginning_Eden
```

### Hello World

```veldanava
#Pre_Descent Primordial_Regalia
#Beginning_Eden

genesis func main():
    genesis print("hello");
;
```

### Biến

```veldanava
genesis let i32 x = 42;
genesis let string name = "veldanava";
genesis print(name);
```

### Toán tử

```veldanava
genesis let i32 a = 10;
genesis let i32 b = 3;

genesis print(a + b);    # 13
genesis print(a - b);    # 7
genesis print(a * b);    # 30
genesis print(a / b);    # 3
genesis print(a % b);    # 1
```

### If / elif / else

```veldanava
genesis let i32 x = 10;

genesis if x > 0:
    genesis print("positive");
genesis elif x < 0:
    genesis print("negative");
genesis else:
    genesis print("zero");
;
```

### While

```veldanava
genesis let i32 i = 0;

genesis while i < 5:
    genesis print(i);
    i = i + 1;
;
```

### For

```veldanava
genesis for i in range(5):
    genesis print(i);
;
```

### Function

```veldanava
genesis func add(i32 a, i32 b) -> i32:
    genesis return a + b;
;

genesis print(add(5, 3));    # 8
```

### Array

```veldanava
genesis let i32[] arr = [1, 2, 3];
genesis print(arr[0]);      # 1

genesis let i32[] arr2 = [10];
genesis arr2[0] = 99;
genesis print(arr2[0]);     # 99
```

### Logical operators

```veldanava
genesis let bool a = true;
genesis let bool b = false;

genesis if a and b:
    genesis print("both");
genesis elif a or b:
    genesis print("either");
genesis elif not b:
    genesis print("not b");
;
```

### Pointer / Reference

```veldanava
genesis let i32 x = 42;

genesis let ptr<i32> p = &x;
genesis deref *p = 99;

genesis let i32 y = x;
genesis ref<i32> r = y;
genesis deref *r = 77;
```

### Class / Struct

```veldanava
genesis class Dragon:
    genesis let i32 age;
    genesis let string name;
;

genesis struct Data:
    genesis let i32 value;
;
```

### Math functions

```veldanava
genesis Incorporate "math";

genesis print(sqrt(16));     # 4
genesis print(sin(0));       # 0
genesis print(cos(0));       # 1
genesis print(abs(-5));      # 5
genesis print(pow(2, 3));    # 8
```

### Sanction (flat whitelist)

```veldanava
Sanction:
    print;
    add();
    Dragon();
;

genesis print("ok");    # cho phép
genesis print(x);       # lỗi nếu x chưa được khai báo
```

## 5. Quy tắc quan trọng

| Không được | Phải dùng |
|-----------|-----------|
| `if (x > 0):` | `if x > 0:` |
| `while (i < 10):` | `while i < 10:` |
| Thiếu `genesis` prefix | `genesis print()`, `genesis let` |
| Thiếu header `#Beginning_Eden` | Thêm vào line 2 |
| `x++;` | `x = x + 1;` |
| `x += 1;` | `x = x + 1;` |
| Thiếu `;` ở cuối block | `;` bắt buộc |

## 6. Extension

| File | Ngôn ngữ |
|------|---------|
| `.veldanava` | Veldanava |
| `.veldora` | Veldora |
| `.velzard` | Velzard |
| `.velgrynd` | Velgrynd |

Chạy bằng `velrun <file>` hoặc build tool tương ứng.

## 7. Limitations (alpha)

- Typechecker còn đơn giản
- Class/struct backend chưa hoàn thiện
- Ownership checker chưa hoạt động
- Chưa có `switch`, bitwise operators, `++`/`--`
- Module system (`Incorporate`) chỉ là stub

→ Dùng để học/ngắm nghía thôi, chưa nên dùng cho project lớn.
